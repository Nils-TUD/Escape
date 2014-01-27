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

#include <esc/common.h>
#include <gui/image/image.h>
#include <gui/button.h>

namespace gui {
	class ImageButton : public Button {
	public:
		ImageButton(std::shared_ptr<Image> img,bool border = true)
			: Button(""), _img(img), _border(border) {
		}
		ImageButton(std::shared_ptr<Image> img,const Pos &pos,const Size &size,bool border = true)
			: Button("",pos,size), _img(img), _border(border) {
		}

		std::shared_ptr<Image> getImage() const {
			return _img;
		}

		virtual void paintBorder(Graphics &g);
		virtual void paintBackground(Graphics &g);

	private:
		virtual Size getPrefSize() const;

		std::shared_ptr<Image> _img;
		bool _border;
	};
}
