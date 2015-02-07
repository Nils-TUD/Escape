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

#include <gui/graphics/graphicsbuffer.h>
#include <gui/application.h>
#include <gui/window.h>
#include <sys/common.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

namespace gui {
	void GraphicsBuffer::allocBuffer() {
		// get window-manager name
		Application *app = Application::getInstance();
		const char *winmng = app->getWinMng();
		const char *p;
		while((p = strchr(winmng,'/')) != NULL)
			winmng = p + 1;

		// attach to shared memory region, created by winmanager
		char name[32];
		snprintf(name, sizeof(name),"%s-win%d",winmng,_win->getId());
		int fd = shm_open(name,O_RDWR,0644);
		if(fd < 0)
			throw esc::default_error(string("Unable to open shm file ") + name,fd);

		size_t bufsize = _size.width * _size.height * (app->getColorDepth() / 8);
		_pixels = static_cast<uint8_t*>(mmap(NULL,bufsize,0,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0));
		close(fd);
		if(_pixels == NULL)
			throw esc::default_error(string("Unable to mmap shm file ") + name,errno);
	}

	void GraphicsBuffer::freeBuffer() {
		if(_pixels) {
			munmap(_pixels);
			_pixels = nullptr;
		}
	}

	void GraphicsBuffer::moveTo(const Pos &pos) {
		Size screenSize = Application::getInstance()->getScreenSize();
		_pos.x = min((gpos_t)screenSize.width - 1,pos.x);
		_pos.y = min((gpos_t)screenSize.height - 1,pos.y);
	}

	void GraphicsBuffer::requestUpdate(const Pos &pos,const Size &size) {
		if(_win->isCreated())
			Application::getInstance()->requestWinUpdate(_win->getId(),pos,size);
	}
}
