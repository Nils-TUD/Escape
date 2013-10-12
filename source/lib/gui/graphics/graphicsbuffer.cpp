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

	void GraphicsBuffer::detach(UIElement *el) {
		if(_lost.el == el)
			_lost.el = nullptr;
	}

	void GraphicsBuffer::lostPaint(UIElement* el,Rectangle rect) {
		// note that this is necessary because we may not be able to repaint multiple rectangles
		// later, since the state of the element might have changed in the meanwhile. therefore,
		// we might repaint the wrong rectangle. so, if we get multiple requests while waiting
		// for an update, we have to be more careful and better repaint more than less.

		// nothing lost yet?
		if(_lost.el == nullptr)
			_lost = LostRepaint(el,rect);
		// if it's the same element, repaint the whole element
		else if(_lost.el == el)
			_lost.rect = Rectangle();
		// otherwise repaint the whole window
		else
			_lost = LostRepaint(el->getWindow(),Rectangle());
	}

	void GraphicsBuffer::onUpdated() {
		_updating = false;
		if(_lost.el) {
			_lost.el->makeDirty(true);
			if(_lost.rect.empty()) {
				// TODO temporary fix for our offset-bug. this prevents paint-errors in e.g. the
				// guishell
				if(_lost.el->getParent())
					_lost.el->getParent()->repaint(true);
				else
					_lost.el->repaint(true);
			}
			else
				_lost.el->repaintRect(_lost.rect.getPos(),_lost.rect.getSize(),true);
			_lost.el = nullptr;
		}
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
