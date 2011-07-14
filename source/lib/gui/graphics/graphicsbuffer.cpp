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

namespace gui {
	void GraphicsBuffer::allocBuffer() {
		switch(_bpp) {
			case 32:
				_pixels = (uint8_t*)calloc(_width * _height,4);
				break;
			case 24:
				_pixels = (uint8_t*)calloc(_width * _height,3);
				break;
			case 16:
				_pixels = (uint8_t*)calloc(_width * _height,2);
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
		gsize_t screenWidth = Application::getInstance()->getScreenWidth();
		gsize_t screenHeight = Application::getInstance()->getScreenHeight();
		_x = MIN(screenWidth - 1,x);
		_y = MIN(screenHeight - 1,y);
	}

	void GraphicsBuffer::resizeTo(gsize_t width,gsize_t height) {
		if(_width != width || _height != height) {
			_width = width;
			_height = height;
			free(_pixels);
			allocBuffer();
		}
	}

	void GraphicsBuffer::requestUpdate(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		if(_win->isCreated()) {
			// if we are the active (=top) window, we can update directly
			if(_win->isActive())
				update(x,y,width,height);
			else {
				// notify winmanager that we want to repaint this area; after a while we'll get multiple
				// (ok, maybe just one) update-events with specific areas to update
				Application::getInstance()->requestWinUpdate(_win->getId(),x,y,width,height);
			}
		}
	}

	void GraphicsBuffer::update(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		// is there anything to update?
		if(width > 0 || height > 0) {
			// validate params
			gsize_t screenWidth = Application::getInstance()->getScreenWidth();
			gsize_t screenHeight = Application::getInstance()->getScreenHeight();
			if(_x + x >= screenWidth || _y + y >= screenHeight)
				return;
			if(_x < 0 && _x + x < 0) {
				width += _x + x;
				x = -_x;
			}
			width = MIN(screenWidth - (x + _x),MIN(_width - x,width));
			height = MIN(screenHeight - (y + _y),MIN(_height - y,height));

			void *vesaMem = Application::getInstance()->getScreenMem();
			uint8_t *src,*dst;
			gpos_t endy = y + height;
			size_t psize = _bpp / 8;
			size_t count = width * psize;
			size_t srcAdd = _width * psize;
			size_t dstAdd = screenWidth * psize;
			src = _pixels + (y * _width + x) * psize;
			dst = (uint8_t*)vesaMem + ((_y + y) * screenWidth + (_x + x)) * psize;
			while(y < endy) {
				memcpy(dst,src,count);
				src += srcAdd;
				dst += dstAdd;
				y++;
			}

			notifyVesa(_x + x,_y + endy - height,width,height);
		}
	}

	void GraphicsBuffer::notifyVesa(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		int vesaFd = Application::getInstance()->getVesaFd();
		sMsg msg;
		if(x < 0) {
			width += x;
			x = 0;
		}
		msg.args.arg1 = x;
		msg.args.arg2 = y;
		msg.args.arg3 = width;
		msg.args.arg4 = height;
		if(send(vesaFd,MSG_VESA_UPDATE,&msg,sizeof(msg.args)) < 0)
			cerr << "Unable to send update-request to VESA" << endl;
	}
}
