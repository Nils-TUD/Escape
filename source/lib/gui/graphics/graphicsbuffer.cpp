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

	void GraphicsBuffer::moveTo(gpos_t x,gpos_t y) {
		Size screenSize = Application::getInstance()->getScreenSize();
		_x = min<gpos_t>(screenSize.width - 1,x);
		_y = min<gpos_t>(screenSize.height - 1,y);
	}

	void GraphicsBuffer::resizeTo(const Size &size) {
		if(_size != size) {
			_size = size;
			free(_pixels);
			allocBuffer();
		}
	}

	void GraphicsBuffer::requestUpdate(gpos_t x,gpos_t y,const Size &size) {
		if(_win->isCreated()) {
			// if we are the active (=top) window, we can update directly
			if(_win->isActive())
				update(x,y,size);
			else {
				// notify winmanager that we want to repaint this area; after a while we'll get multiple
				// (ok, maybe just one) update-events with specific areas to update
				Application::getInstance()->requestWinUpdate(_win->getId(),x,y,size);
			}
		}
	}

	void GraphicsBuffer::update(gpos_t x,gpos_t y,const Size &size) {
		// is there anything to update?
		if(!size.empty()) {
			Size rsize = size;
			// validate params
			Size screenSize = Application::getInstance()->getScreenSize();
			if(_x + x >= screenSize.width || _y + y >= screenSize.height)
				return;
			if(_x < 0 && _x + x < 0) {
				rsize.width += _x + x;
				x = -_x;
			}
			rsize.width = MIN(screenSize.width - (x + _x),MIN(_size.width - x,rsize.width));
			rsize.height = MIN(screenSize.height - (y + _y),MIN(_size.height - y,rsize.height));

			void *vesaMem = Application::getInstance()->getScreenMem();
			uint8_t *src,*dst;
			gpos_t endy = y + rsize.height;
			size_t psize = _bpp / 8;
			size_t count = rsize.width * psize;
			size_t srcAdd = _size.width * psize;
			size_t dstAdd = screenSize.width * psize;
			src = _pixels + (y * _size.width + x) * psize;
			dst = (uint8_t*)vesaMem + ((_y + y) * screenSize.width + (_x + x)) * psize;
			while(y < endy) {
				memcpy(dst,src,count);
				src += srcAdd;
				dst += dstAdd;
				y++;
			}

			notifyVesa(_x + x,_y + endy - rsize.height,rsize);
		}
	}

	void GraphicsBuffer::notifyVesa(gpos_t x,gpos_t y,const Size &size) {
		Size rsize = size;
		int vesaFd = Application::getInstance()->getVesaFd();
		sMsg msg;
		if(x < 0) {
			rsize.width += x;
			x = 0;
		}
		msg.args.arg1 = x;
		msg.args.arg2 = y;
		msg.args.arg3 = rsize.width;
		msg.args.arg4 = rsize.height;
		if(send(vesaFd,MSG_VESA_UPDATE,&msg,sizeof(msg.args)) < 0)
			cerr << "Unable to send update-request to VESA" << endl;
	}
}
