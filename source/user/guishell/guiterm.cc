/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <gui/window.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/thread.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include <algorithm>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "guiterm.h"

GUIVTermDevice::GUIVTermDevice(const char *path,mode_t mode,std::shared_ptr<ShellControl> sh,
		uint cols,uint rows)
	: esc::VTermDevice(path,mode,&_vt), _vt(), _mode(), _sh(sh),
	  _rbuffer(new char[READ_BUF_SIZE + 1]), _rbufPos(0) {

	set(MSG_SCR_GETMODE,std::make_memfun(this,&GUIVTermDevice::getMode));
	set(MSG_SCR_GETMODES,std::make_memfun(this,&GUIVTermDevice::getModes));
	set(MSG_FILE_WRITE,std::make_memfun(this,&GUIVTermDevice::write));
	unset(MSG_VT_SETMODE);
	/* TODO MSG_UIM_{GET,SET}KEYMAP are not supported yet */

	// open speaker
	try {
		_vt.speaker = new esc::Speaker("/dev/speaker");
	}
	catch(const std::exception &e) {
		/* ignore errors here. in this case we simply don't use it */
		printe("%s",e.what());
	}

	_vt.data = this;
	_vt.index = 0;
	_vt.sid = id();
	_vt.defForeground = BLACK;
	_vt.defBackground = WHITE;
	_vt.setCursor = setCursor;
	if(getenvto(_vt.name,sizeof(_vt.name),"TERM") < 0)
		error("Unable to get env-var TERM");

	_mode.cols = cols;
	_mode.rows = rows;
	_mode.type = esc::Screen::MODE_TYPE_TUI;
	_mode.mode = esc::Screen::MODE_GRAPHICAL;
	if(!vtctrl_init(&_vt,&_mode))
		error("Unable to init vterm");

	_sh->setVTerm(&_vt);
	_sh->setDevice(this);
}

GUIVTermDevice::~GUIVTermDevice() {
	delete _vt.speaker;
	delete[] _rbuffer;
	vtctrl_destroy(&_vt);
}

void GUIVTermDevice::loop() {
	ulong buf[IPC_DEF_SIZE / sizeof(ulong)];
	while(!isStopped()) {
		msgid_t mid;
		int fd = getwork(id(),&mid,buf,sizeof(buf),_rbufPos > 0 ? GW_NOBLOCK : 0);
		if(EXPECT_FALSE(fd < 0)) {
			/* just log that it failed. maybe a client has sent a message that was too big */
			if(fd != -EINTR && fd != -ENOCLIENT)
				printe("getwork failed");

			/* append the buffer now to reduce delays */
			if(_rbufPos > 0) {
				{
					std::lock_guard<std::mutex> guard(*_vt.mutex);
					_rbuffer[_rbufPos] = '\0';
					vtout_puts(&_vt,_rbuffer,_rbufPos,true);
					_sh->update();
					_rbufPos = 0;
				}
				checkPending();
			}
			continue;
		}

		esc::IPCStream is(fd,buf,sizeof(buf),mid);
		handleMsg(mid,is);
		_sh->update();
	}
}

void GUIVTermDevice::getMode(esc::IPCStream &is) {
	prepareMode();
	is << 0 << _mode << esc::Reply();
}

void GUIVTermDevice::getModes(esc::IPCStream &is) {
	size_t n;
	is >> n;

	is << static_cast<ssize_t>(1) << esc::Reply();
	if(n) {
		prepareMode();
		is << esc::ReplyData(&_mode,sizeof(esc::Screen::Mode));
	}
}

void GUIVTermDevice::write(esc::IPCStream &is) {
	esc::FileWrite::Request r;
	is >> r;
	assert(!is.error());

	char *data = r.count <= BUF_SIZE ? _buffer : (char*)malloc(r.count + 1);
	ssize_t res = 0;
	if(data) {
		is >> esc::ReceiveData(data,r.count);
		data[r.count] = '\0';
		res = r.count;

		char *dataWork = data;
		while(r.count > 0) {
			size_t amount = std::min(r.count,READ_BUF_SIZE - _rbufPos);
			memcpy(_rbuffer + _rbufPos,dataWork,amount);

			r.count -= amount;
			_rbufPos += amount;
			dataWork += amount;
			if(_rbufPos >= READ_BUF_SIZE) {
				std::lock_guard<std::mutex> guard(*_vt.mutex);
				_rbuffer[_rbufPos] = '\0';
				vtout_puts(&_vt,_rbuffer,_rbufPos,true);
				_rbufPos = 0;
			}
		}

		if(r.count > BUF_SIZE)
			free(data);
	}

	is << esc::FileWrite::Response::result(res) << esc::Reply();
	checkPending();
}

void GUIVTermDevice::setCursor(sVTerm *vt) {
	if(vt->col != vt->lastCol || vt->row != vt->lastRow) {
		static_cast<GUIVTermDevice*>(vt->data)->_sh->setCursor();
		vt->lastCol = vt->col;
		vt->lastRow = vt->row;
	}
}

void GUIVTermDevice::prepareMode() {
	gui::Application *app = gui::Application::getInstance();
	_mode.bitsPerPixel = app->getColorDepth();
	_mode.cols = _vt.cols;
	_mode.rows = _vt.rows;
	_mode.width = _sh->getWindow()->getSize().width;
	_mode.height = _sh->getWindow()->getSize().height;
}
