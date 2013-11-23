/**
 * $Id$
 */

#include <esc/common.h>
#include <gui/graphics/graphicsbuffer.h>
#include <gui/application.h>
#include <gui/window.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/mem.h>
#include <stdlib.h>
#include <iostream>

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
		_fd = shm_open(name,IO_READ | IO_WRITE,0644);
		if(_fd < 0)
			throw std::io_exception(string("Unable to open shm file ") + name,_fd);

		size_t bufsize = _size.width * _size.height * (app->getColorDepth() / 8);
		_pixels = static_cast<uint8_t*>(mmap(NULL,bufsize,0,PROT_READ | PROT_WRITE,MAP_SHARED,_fd,0));
		if(_pixels == NULL) {
			close(_fd);
			throw std::io_exception(string("Unable to mmap shm file ") + name,errno);
		}
	}

	void GraphicsBuffer::freeBuffer() {
		if(_pixels) {
			munmap(_pixels);
			close(_fd);
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
