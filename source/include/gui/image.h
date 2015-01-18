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

#include <gui/graphics/graphics.h>
#include <img/image.h>
#include <sys/common.h>
#include <exception>
#include <memory>

namespace gui {
	class GUIPainter : public img::Painter {
	public:
		void reset(Graphics *g,const Pos &pos) {
			_g = g;
			_last = 0;
			_g->setColor(Color(_last));
			_pos = pos;
		}

		virtual void paintPixel(gpos_t x,gpos_t y,uint32_t col) {
			if(EXPECT_FALSE(col != _last)) {
				_g->setColor(Color(col));
				_last = col;
			}
			if(!_g->getColor().isTransparent())
				_g->doSetPixel(x + _pos.x,y + _pos.y);
		}

	private:
		Graphics *_g;
		uint32_t _last;
		Pos _pos;
	};

	class Image {
	public:
		static std::shared_ptr<Image> loadImage(const std::string& path) {
			return std::shared_ptr<Image>(
				new Image(img::Image::loadImage(std::shared_ptr<GUIPainter>(new GUIPainter()),path)));
		}

		explicit Image(img::Image *img)
			: _painter(std::static_pointer_cast<GUIPainter>(img->getPainter())), _img(img) {
		}

		Size getSize() const {
			Size res;
			_img->getSize(&res.width,&res.height);
			return res;
		}

		void paint(Graphics &g,const Pos &pos);

	private:
		std::shared_ptr<GUIPainter> _painter;
		std::shared_ptr<img::Image> _img;
	};
}
