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

#include <esc/proto/ui.h>
#include <esc/vthrow.h>
#include <sys/common.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>
#include <memory>
#include <stdlib.h>

#include "clients.h"
#include "header.h"

size_t UIClient::_active = MAX_CLIENTS;
UIClient *UIClient::_clients[MAX_CLIENTS];
UIClient *UIClient::_allclients[MAX_CLIENTS];
std::vector<UIClient*> UIClient::_clientStack;
size_t UIClient::_clientCount = 0;

UIClient::UIClient(int f)
	: Client(f), _idx(-1), _evfd(-1), _map(), _screen(),
	  _fb(), _header(), _cursor(), _type(esc::Screen::MODE_TYPE_TUI) {
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
}

UIClient::~UIClient() {
	delete _fb;
	delete[] _header;
}

void UIClient::setActive(bool active) {
	esc::UIEvents::Event ev;
	ev.type = active ? esc::UIEvents::Event::TYPE_UI_ACTIVE : esc::UIEvents::Event::TYPE_UI_INACTIVE;
	::send(_evfd,MSG_UIM_EVENT,&ev,sizeof(ev));

	if(!active) {
		_clientStack.erase_first(this);
		_clientStack.insert(_clientStack.begin(),this);
	}
}

void UIClient::reactivate(UIClient *cli,UIClient *old,int oldMode) {
	if(cli == NULL || !cli->_fb)
		return;

	if(old && old != cli)
		old->setActive(false);

	/* we might have send e.g. some update-messages which haven't been handled yet and of course we
	 * don't want the screen-driver to handle them afterwards since that would overwrite something
	 * of the new client. thus, perform a send-receive to be sure that no update-requests (without
	 * reply) are pending. */
	if(old && old->_fb)
		old->_screen->getModeCount();

	cli->_screen->setMode(cli->_type,cli->modeid(),cli->_fb->filename().c_str(),oldMode != cli->modeid());
	cli->_screen->setCursor(cli->_cursor.x,cli->_cursor.y,cli->_cursor.cursor);

	/* now everything is complete, so set the new active client */
	_active = cli->_idx;

	gsize_t w = cli->_type == esc::Screen::MODE_TYPE_TUI ? cli->_fb->mode().cols : cli->_fb->mode().width;
	gsize_t h = cli->_type == esc::Screen::MODE_TYPE_TUI ? cli->_fb->mode().rows : cli->_fb->mode().height;
	gsize_t dw,dh;
	Header::update(cli,&dw,&dh);
	cli->_screen->update(0,0,w,h);

	if(old != cli)
		cli->setActive(true);
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

void UIClient::setMode(int ntype,const esc::Screen::Mode &mode,esc::Screen *scr,const char *file,bool set) {
	/* join framebuffer; this might throw */
	std::unique_ptr<esc::FrameBuffer> nfb(new esc::FrameBuffer(mode,file,ntype));

	/* set mode; might throw, too */
	if(set)
		scr->setMode(ntype,mode.id,file,true);

	/* destroy old stuff */
	delete[] _header;
	delete _fb;

	/* set new stuff */
	_header = new char[
		Header::getSize(mode,ntype,ntype == esc::Screen::MODE_TYPE_TUI ? mode.cols : mode.width)];
	_screen = scr;
	_fb = nfb.release();
	_type = ntype;
}

void UIClient::setCursor(gpos_t x,gpos_t y,int cursor) {
	if(screen()) {
		_cursor.x = x;
		_cursor.y = y + Header::getHeight(type());
		_cursor.cursor = cursor;
		screen()->setCursor(_cursor.x,_cursor.y,_cursor.cursor);
	}
}

void UIClient::send(const void *msg,size_t size) {
	/* if the client is not attached yet, don't send him messages */
	if(_active == MAX_CLIENTS || _clients[_active]->_idx == static_cast<size_t>(-1))
		return;
	::send(_clients[_active]->_evfd,MSG_UIM_EVENT,msg,size);
}

int UIClient::attach(int evfd) {
	if(_evfd != -1)
		VTHROWE("There is already an event-channel",-EEXIST);
	if(_clientCount >= MAX_CLIENTS)
		VTHROW("Max. number of clients reached");

	for(size_t i = 0; i < MAX_CLIENTS; ++i) {
		if(_clients[i] == NULL) {
			if(_active != MAX_CLIENTS)
				_clients[_active]->setActive(false);

			_clients[i] = this;
			_evfd = evfd;
			_idx = i;
			_active = i;
			_clientCount++;
			setActive(true);
			return i;
		}
	}
	A_UNREACHED;
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
	if(_evfd != -1)
		close(_evfd);
	_clients[_idx] = NULL;
	_evfd = -1;
	_idx = -1;
	_clientCount--;
	_clientStack.erase_first(this);
	/* do that here because we need to remove us from the list first */
	if(shouldSwitch && _clientStack.size() > 0) {
		do {
			UIClient *last = _clientStack.front();
			if(last->_idx != MAX_CLIENTS) {
				switchTo(last->_idx);
				break;
			}
			_clientStack.erase(_clientStack.begin());
		}
		while(_clientStack.size() > 0);
	}
}
