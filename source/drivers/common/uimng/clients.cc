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

#include <esc/common.h>
#include <esc/sllist.h>
#include <esc/mem.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <vthrow.h>
#include <memory>

#include "clients.h"
#include "header.h"

size_t UIClient::_active = MAX_CLIENTS;
UIClient *UIClient::_clients[MAX_CLIENTS];
UIClient *UIClient::_allclients[MAX_CLIENTS];
size_t UIClient::_clientCount = 0;

UIClient::UIClient(int f)
	: Client(f), _idx(-1), _evfd(-1), _randid(-1), _map(), _screen(),
	  _fb(), _header(), _cursor(), _type(VID_MODE_TYPE_TUI) {
	UIClient *dummy;
	do {
		_randid = rand();
	}
	while(getByRandId(_randid,&dummy) == 0);

	bool succ = false;
	for(size_t i = 0; i < MAX_CLIENTS; ++i) {
		if(_allclients[i] == NULL) {
			_allclients[i] = this;
			succ = true;
			break;
		}
	}

	if(!succ)
		VTHROW("Max. number of clients reached");

	_cursor.cursor = CURSOR_DEFAULT;
}

UIClient::~UIClient() {
	delete _fb;
	delete[] _header;
}

void UIClient::reactivate(UIClient *cli,UIClient *old,int oldMode) {
	if(cli == NULL || !cli->_fb)
		return;

	/* before switching, discard all messages that are in flight from the old client. because we
	 * might have send e.g. some update-messages which haven't been handled yet and of course we
	 * don't want the screen-driver to handle them afterwards since that would overwrite something
	 * of the new client. note that this is no problem because if we switch back to the old client,
	 * we'll update everything anyway. */
	if(old && old->_fb)
		fcntl(old->_screen->fd(),F_DISMSGS,0);

	cli->_screen->setMode(cli->_type,cli->modeid(),cli->_fb->filename().c_str(),oldMode != cli->modeid());
	cli->_screen->setCursor(cli->_cursor.x,cli->_cursor.y,cli->_cursor.cursor);

	/* now everything is complete, so set the new active client */
	_active = cli->_idx;

	gsize_t w = cli->_type == VID_MODE_TYPE_TUI ? cli->_fb->mode().cols : cli->_fb->mode().width;
	gsize_t h = cli->_type == VID_MODE_TYPE_TUI ? cli->_fb->mode().rows : cli->_fb->mode().height;
	gsize_t dw,dh;
	header_update(cli,&dw,&dh);
	cli->_screen->update(0,0,w,h);
}

void UIClient::switchClient(int incr) {
	int oldMode = getOldMode();
	UIClient *oldcli = getActive();
	size_t newActive,oldActive = _active;
	/* set current client to nothing */
	_active = MAX_CLIENTS;

	newActive = oldActive;
	if(oldActive == MAX_CLIENTS)
		newActive = 0;
	if(_clientCount > 0) {
		do {
			newActive = (newActive + incr) % MAX_CLIENTS;
		}
		while(_clients[newActive] == NULL || _clients[newActive]->_fb == NULL);
	}
	else
		newActive = MAX_CLIENTS;

	if(newActive != MAX_CLIENTS && newActive != oldActive)
		reactivate(_clients[newActive],oldcli,oldMode);
	else
		_active = newActive;
}

void UIClient::setMode(int ntype,const ipc::Screen::Mode &mode,ipc::Screen *scr,const char *file,bool set) {
	/* join framebuffer; this might throw */
	std::unique_ptr<ipc::FrameBuffer> nfb(new ipc::FrameBuffer(mode,file,ntype));

	/* set mode; might throw, too */
	if(set)
		scr->setMode(ntype,mode.id,file,true);

	/* destroy old stuff */
	delete[] _header;
	delete _fb;

	/* set new stuff */
	_header = new char[
		header_getSize(mode,ntype,ntype == VID_MODE_TYPE_TUI ? mode.cols : mode.width)];
	_screen = scr;
	_fb = nfb.release();
	_type = ntype;
}

void UIClient::setCursor(gpos_t x,gpos_t y,int cursor) {
	_cursor.x = x;
	_cursor.y = y + header_getHeight(type());
	_cursor.cursor = cursor;
	screen()->setCursor(_cursor.x,_cursor.y,_cursor.cursor);
}

void UIClient::send(const void *msg,size_t size) {
	/* if the client is not attached yet, don't send him messages */
	if(_active == MAX_CLIENTS || _clients[_active]->_idx == static_cast<size_t>(-1))
		return;
	::send(_clients[_active]->_evfd,MSG_UIM_EVENT,msg,size);
}

int UIClient::getByRandId(int randid,UIClient **cli) {
	*cli = NULL;
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		if(_allclients[i] && _allclients[i]->_randid == randid) {
			if(*cli)
				return -EEXIST;
			*cli = _allclients[i];
		}
	}
	return *cli == NULL ? -EINVAL : 0;
}

size_t UIClient::attach(int randid,int evfd) {
	UIClient *cli;
	if(getByRandId(randid,&cli) != 0)
		VTHROW("No client with randid " << randid);
	if(_clientCount >= MAX_CLIENTS)
		VTHROW("Max. number of clients reached");

	for(size_t i = 0; i < MAX_CLIENTS; ++i) {
		if(_clients[i] == NULL) {
			_clients[i] = cli;
			cli->_evfd = evfd;
			cli->_idx = i;
			_active = i;
			_clientCount++;
			return i;
		}
	}
	A_UNREACHED;
}

void UIClient::detach(int evfd) {
	for(size_t i = 0; i < MAX_CLIENTS; ++i) {
		if(_allclients[i] && _allclients[i]->_evfd == evfd)
			_allclients[i] = NULL;
		if(_clients[i] && _clients[i]->_evfd == evfd)
			_clients[i]->remove();
	}
}

void UIClient::remove() {
	bool shouldSwitch = false;
	if(isActive()) {
		_active = MAX_CLIENTS;
		shouldSwitch = true;
	}
	for(size_t i = 0; i < MAX_CLIENTS; ++i) {
		if(_allclients[i] == this)
			_allclients[i] = NULL;
	}
	_clients[_idx] = NULL;
	_evfd = -1;
	_idx = -1;
	_clientCount--;
	/* do that here because we need to remove us from the list first */
	if(shouldSwitch)
		next();
}
