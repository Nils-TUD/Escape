/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <gui/graphics/color.h>
#include <gui/control.h>
#include <string>

namespace gui {
	class ProgressBar : public Control {
	public:
		ProgressBar(const std::string &text)
			: Control(), _position(0), _text(text) {
		};
		ProgressBar(const std::string &text,gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: Control(x,y,width,height), _position(0), _text(text) {
		};

		inline size_t getPosition() const {
			return _position;
		};
		inline void setPosition(size_t pos) {
			_position = std::min((size_t)100,pos);
			repaint();
		};
		inline const std::string &getText() const {
			return _text;
		};
		inline void setText(const std::string &text) {
			_text = text;
			repaint();
		};

		virtual gsize_t getMinWidth() const;
		virtual gsize_t getMinHeight() const;

	protected:
		virtual void paint(Graphics &g);

	private:
		size_t _position;
		std::string _text;
	};
}
