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

#pragma once

#include <stdlib.h>

#include "../painter/ui.h"

class GUIPainter : public UIPainter {
public:
	explicit GUIPainter(gsize_t width,gsize_t height) : UIPainter() {
		/* find desired mode */
		mode = ui->findGraphicsMode(width,height,32);
		if(mode.bitsPerPixel != 32)
			error("Mode %ux%ux%u is not supported",mode.width,mode.height,mode.bitsPerPixel);

		/* create framebuffer and set mode */
		fb = new esc::FrameBuffer(mode,esc::Screen::MODE_TYPE_GUI);
		ui->setMode(esc::Screen::MODE_TYPE_GUI,mode.id,fb->fd(),true);

		ui->hideCursor();

		/* start */
		resize(mode.width,mode.height);
		start();
	}

	virtual void set(const RGB &add,gpos_t x,gpos_t y,size_t factor,float val) {
		uint32_t rgba = 0;
		rgba |= (uint32_t)(255 * (.5f + .5f * fastsin(val + add.r))) << 0;
		rgba |= (uint32_t)(255 * (.5f + .5f * fastsin(val + add.g))) << 8;
		rgba |= (uint32_t)(255 * (.5f + .5f * fastsin(val + add.b))) << 16;
		rgba |= (uint32_t)(255) << 24;

		uint32_t *px = (uint32_t*)fb->addr() + y * factor * width + x * factor;
		size_t ynum = esc::Util::min(factor,height - factor * y);
		size_t xnum = esc::Util::min(factor,width - factor * x);
		for(size_t j = 0; j < ynum; ++j) {
			for(size_t i = 0; i < xnum; ++i)
				px[j * width + i] = rgba;
		}
	}
};
