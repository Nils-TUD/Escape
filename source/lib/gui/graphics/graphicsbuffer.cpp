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
		if(!_win->isCreated())
			return;

		// attach to shared memory region, created by winmanager
		char name[16];
		snprintf(name, sizeof(name), "win-%d", _win->getId());
		_fd = shm_open(name,IO_READ | IO_WRITE,0644);
		if(_fd < 0)
			throw std::io_exception(string("Unable to open shm file ") + name, _fd);
		_pixels = static_cast<uint8_t*>(mmap(NULL,_size.width * _size.height * (_bpp / 8),0,
				PROT_READ | PROT_WRITE,MAP_SHARED,_fd,0));
		if(_pixels == NULL) {
			close(_fd);
			throw std::io_exception(string("Unable to mmap shm file ") + name, errno);
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

	bool GraphicsBuffer::onUpdated() {
		bool res = _lostpaint;
		_updating = false;
		_lostpaint = false;
		return res;
	}

	void GraphicsBuffer::requestUpdate(const Pos &pos,const Size &size) {
		if(!_updating && _win->isCreated()) {
			Application::getInstance()->requestWinUpdate(_win->getId(),pos,size);
			// don't touch our buffer until the update has been done. otherwise we might see
			// half-finished paints on the screen. if the application tries to paint something
			// in the meanwhile, we note that (_lostpaint) and repaint afterwards.
			_updating = true;
		}
	}
}
