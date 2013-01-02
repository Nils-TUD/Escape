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
#include <gui/event/subscriber.h>
#include <gui/control.h>
#include <string>

namespace gui {
	class Button : public Control {
	public:
		typedef Sender<UIElement&> onclick_type;

		Button(const std::string &text)
			: Control(), _focused(false), _pressed(false), _text(text), _clicked() {
		}
		Button(const std::string &text,const Pos &pos,const Size &size)
			: Control(pos,size), _focused(false), _pressed(false), _text(text), _clicked() {
		}

		bool isPressed() const {
			return _pressed;
		}
		const std::string &getText() const {
			return _text;
		}
		void setText(const std::string &text) {
			_text = text;
			repaint();
		}

		virtual Size getPrefSize() const;
		virtual void onFocusGained();
		virtual void onFocusLost();
		virtual void onKeyPressed(const KeyEvent &e);
		virtual void onKeyReleased(const KeyEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);

		onclick_type &clicked() {
			return _clicked;
		}

	protected:
		virtual void paintBackground(Graphics &g);
		virtual void paintBorder(Graphics &g);
		virtual void paint(Graphics &g);

	private:
		void setPressed(bool pressed);

	private:
		bool _focused;
		bool _pressed;
		std::string _text;
		onclick_type _clicked;
	};
}
