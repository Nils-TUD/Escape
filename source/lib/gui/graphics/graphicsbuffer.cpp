/**
 * $Id$
 */

#include <esc/common.h>
#include <gui/graphics/graphicsbuffer.h>
#include <gui/application.h>
#include <gui/window.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

namespace gui {
	void GraphicsBuffer::allocBuffer() {
		if(!_win->isCreated())
			return;

		switch(_bpp) {
			case 32:
				_pixels = (uint8_t*)calloc(_size.width * _size.height,4);
				break;
			case 24:
				_pixels = (uint8_t*)calloc(_size.width * _size.height,3);
				break;
			case 16:
				_pixels = (uint8_t*)calloc(_size.width * _size.height,2);
				break;
			default:
				cerr << "Unsupported color-depth: " << _bpp << endl;
				exit(EXIT_FAILURE);
				break;
		}
		if(!_pixels) {
			cerr << "Not enough memory" << endl;
			exit(EXIT_FAILURE);
		}
	}

	void GraphicsBuffer::moveTo(const Pos &pos) {
		Size screenSize = Application::getInstance()->getScreenSize();
		_pos.x = min<gpos_t>(screenSize.width - 1,pos.x);
		_pos.y = min<gpos_t>(screenSize.height - 1,pos.y);
	}

	void GraphicsBuffer::resizeTo(const Size &size) {
		if(_size != size) {
			_size = size;
			free(_pixels);
			allocBuffer();
		}
	}

	void GraphicsBuffer::requestUpdate(const Pos &pos,const Size &size) {
		if(_win->isCreated()) {
			// if we are the active (=top) window, we can update directly
			if(_win->isActive())
				update(pos,size);
			else {
				// notify winmanager that we want to repaint this area; after a while we'll get multiple
				// (ok, maybe just one) update-events with specific areas to update
				Application::getInstance()->requestWinUpdate(_win->getId(),pos,size);
			}
		}
	}

	void GraphicsBuffer::update(const Pos &pos,const Size &size) {
		// is there anything to update?
		if(_pixels && !size.empty()) {
			Size rsize = size;
			Pos rpos = pos;
			// validate params
			Size screenSize = Application::getInstance()->getScreenSize();
			if(_pos.x + rpos.x >= screenSize.width || _pos.y + rpos.y >= screenSize.height)
				return;
			if(_pos.x < 0 && _pos.x + rpos.x < 0) {
				rsize.width += _pos.x + rpos.x;
				rpos.x = -_pos.x;
			}
			rsize.width = min<gsize_t>(screenSize.width - (rpos.x + _pos.x),
					min<gsize_t>(_size.width - rpos.x,rsize.width));
			rsize.height = min<gsize_t>(screenSize.height - (rpos.y + _pos.y),
					min<gsize_t>(_size.height - rpos.y,rsize.height));

			void *vesaMem = Application::getInstance()->getScreenMem();
			uint8_t *src,*dst;
			gpos_t endy = rpos.y + rsize.height;
			size_t psize = _bpp / 8;
			size_t count = rsize.width * psize;
			size_t srcAdd = _size.width * psize;
			size_t dstAdd = screenSize.width * psize;
			src = _pixels + (rpos.y * _size.width + rpos.x) * psize;
			dst = (uint8_t*)vesaMem + ((_pos.y + rpos.y) * screenSize.width + (_pos.x + rpos.x)) * psize;
			while(rpos.y < endy) {
				memcpy(dst,src,count);
				src += srcAdd;
				dst += dstAdd;
				rpos.y++;
			}

			notifyVesa(Pos(_pos.x + rpos.x,_pos.y + endy - rsize.height),rsize);
		}
	}

	void GraphicsBuffer::notifyVesa(const Pos &pos,const Size &size) {
		Pos rpos = pos;
		Size rsize = size;
		int vesaFd = Application::getInstance()->getVesaFd();
		sMsg msg;
		if(rpos.x < 0) {
			rsize.width += rpos.x;
			rpos.x = 0;
		}
		msg.args.arg1 = rpos.x;
		msg.args.arg2 = rpos.y;
		msg.args.arg3 = rsize.width;
		msg.args.arg4 = rsize.height;
		if(send(vesaFd,MSG_VESA_UPDATE,&msg,sizeof(msg.args)) < 0)
			cerr << "Unable to send update-request to VESA" << endl;
	}
}
