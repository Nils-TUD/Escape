/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <esc/common.h>
#include <vterm/vtout.h>
#include <vterm/vtin.h>
#include <esc/driver/vterm.h>
#include <esc/driver.h>
#include <esc/thread.h>
#include <gui/window.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "guiterm.h"

using namespace std;

GUITerm::GUITerm(int sid,shared_ptr<ShellControl> sh)
	: _sid(sid), _run(true), _vt(nullptr), _mode(), _sh(sh),
	  _rbuffer(new char[READ_BUF_SIZE + 1]), _rbufPos(0) {
	// create and init vterm
	_vt = (sVTerm*)malloc(sizeof(sVTerm));
	if(!_vt)
		error("Not enough mem for vterm");

	// open speaker
	_vt->speaker = open("/dev/speaker",IO_MSGS);
	if(_vt->speaker < 0)
		error("Unable to open '/dev/speaker'");

	_vt->data = this;
	_vt->index = 0;
	_vt->sid = sid;
	_vt->defForeground = BLACK;
	_vt->defBackground = WHITE;
	_vt->setCursor = setCursor;
	if(getenvto(_vt->name,sizeof(_vt->name),"TERM") < 0)
		error("Unable to get env-var TERM");

	_mode.cols = DEF_COLS;
	_mode.rows = DEF_ROWS;
	_mode.type = VID_MODE_TYPE_TUI;
	_mode.mode = VID_MODE_GRAPHICAL;
	if(!vtctrl_init(_vt,&_mode))
		error("Unable to init vterm");

	_sh->setVTerm(_vt);
}

GUITerm::~GUITerm() {
	delete[] _rbuffer;
	vtctrl_destroy(_vt);
	free(_vt);
	close(_sid);
}

void GUITerm::setCursor(sVTerm *vt) {
	gpos_t x,y;
	if(vt->upScroll != 0 || vt->col != vt->lastCol || vt->row != vt->lastRow) {
		/* draw no cursor if it's not visible by setting it to an invalid location */
		if(vt->firstVisLine + vt->rows <= vt->currLine + vt->row) {
			x = vt->cols;
			y = vt->rows;
		}
		else {
			x = vt->col;
			y = vt->row;
		}
		static_cast<GUITerm*>(vt->data)->_sh->setCursor();
		vt->lastCol = x;
		vt->lastRow = y;
	}
}

void GUITerm::prepareMode() {
	gui::Application *app = gui::Application::getInstance();
	_mode.bitsPerPixel = app->getColorDepth();
	_mode.width = _sh->getWindow()->getSize().width;
	_mode.height = _sh->getWindow()->getSize().height;
}

void GUITerm::run() {
	sMsg msg;
	msgid_t mid;
	while(_run) {
		int fd = getwork(_sid,&mid,&msg,sizeof(msg),GW_NOBLOCK);
		if(fd < 0) {
			if(fd != -ENOCLIENT)
				printe("[GUISH] Unable to get client");
			// append the buffer now to reduce delays
			if(_rbufPos > 0) {
				_rbuffer[_rbufPos] = '\0';
				vtout_puts(_vt,_rbuffer,_rbufPos,true);
				_sh->update();
				_rbufPos = 0;
			}
			wait(EV_CLIENT,_sid);
		}
		else {
			switch(mid) {
				case MSG_DEV_READ:
					read(fd,&msg);
					break;

				case MSG_DEV_WRITE:
					write(fd,&msg);
					break;

				case MSG_SCR_GETMODES: {
					if(msg.args.arg1 == 0) {
						msg.args.arg1 = 1;
						send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					}
					else {
						prepareMode();
						send(fd,MSG_DEF_RESPONSE,&_mode,sizeof(_mode));
					}
				}
				break;

				case MSG_SCR_GETMODE: {
					msg.data.arg1 = 0;
					prepareMode();
					memcpy(msg.data.d,&_mode,sizeof(_mode));
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
				}
				break;

				case MSG_VT_SHELLPID:
				case MSG_VT_EN_ECHO:
				case MSG_VT_DIS_ECHO:
				case MSG_VT_EN_RDLINE:
				case MSG_VT_DIS_RDLINE:
				case MSG_VT_EN_NAVI:
				case MSG_VT_DIS_NAVI:
				case MSG_VT_BACKUP:
				case MSG_VT_RESTORE:
				case MSG_VT_ISVTERM:
					msg.data.arg1 = vtctrl_control(_vt,mid,msg.data.d);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
					break;

				case MSG_UIM_GETKEYMAP:
				case MSG_UIM_SETKEYMAP:
				case MSG_VT_SETMODE:
					/* TODO */
				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			_sh->update();
		}
	}
}

void GUITerm::read(int fd,sMsg *msg) {
	// offset is ignored here
	size_t count = msg->args.arg2;
	char *data = (char*)malloc(count);
	int avail;
	msg->args.arg1 = vtin_gets(_vt,data,count,&avail);
	msg->args.arg2 = avail;
	send(fd,MSG_DEV_READ_RESP,msg,sizeof(msg->args));
	if(msg->args.arg1) {
		send(fd,MSG_DEV_READ_RESP,data,msg->args.arg1);
		free(data);
	}
}

void GUITerm::write(int fd,sMsg *msg) {
	msgid_t mid;
	size_t amount;
	size_t c = msg->args.arg2;
	char *data = (char*)malloc(c + 1);
	msg->args.arg1 = 0;
	if(data) {
		if(IGNSIGS(receive(fd,&mid,data,c + 1)) >= 0) {
			char *dataWork = data;
			msg->args.arg1 = c;
			dataWork[c] = '\0';
			while(c > 0) {
				amount = MIN(c,READ_BUF_SIZE - _rbufPos);
				memcpy(_rbuffer + _rbufPos,dataWork,amount);

				c -= amount;
				_rbufPos += amount;
				dataWork += amount;
				if(_rbufPos >= READ_BUF_SIZE) {
					_rbuffer[_rbufPos] = '\0';
					vtout_puts(_vt,_rbuffer,_rbufPos,true);
					_rbufPos = 0;
				}
			}
		}
		free(data);
	}
	send(fd,MSG_DEV_WRITE_RESP,msg,sizeof(msg->args));
}
