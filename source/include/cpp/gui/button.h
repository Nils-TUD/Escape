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

#ifndef BUTTON_H_
#define BUTTON_H_

#include <esc/common.h>
#include <gui/control.h>
#include <gui/color.h>
#include <gui/eventlist.h>
#include <gui/actionlistener.h>
#include <string>

namespace gui {
	class Button : public Control, public EventList<ActionListener> {
	private:
		static Color BGCOLOR;
		static Color FGCOLOR;
		static Color LIGHT_BORDER_COLOR;
		static Color DARK_BORDER_COLOR;

	public:
		Button(gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: Control(x,y,width,height), _focused(false), _pressed(false), _text(string()) {
		};
		Button(const string &text,gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: Control(x,y,width,height), _focused(false), _pressed(false), _text(text) {
		};
		Button(const Button &b)
			: Control(b), _focused(false), _pressed(b._pressed), _text(b._text) {
		};
		virtual ~Button() {

		};
		Button &operator=(const Button &b);

		inline bool isPressed() const {
			return _pressed;
		};
		inline string getText() const {
			return _text;
		};
		inline void setText(const string &text) {
			_text = text;
			repaint();
		};

		virtual void onFocusGained();
		virtual void onFocusLost();
		virtual void onKeyPressed(const KeyEvent &e);
		virtual void onKeyReleased(const KeyEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void paintBackground(Graphics &g);
		virtual void paint(Graphics &g);

	private:
		void notifyListener();
		void setPressed(bool pressed);

	private:
		bool _focused;
		bool _pressed;
		string _text;
	};
}

#endif /* BUTTON_H_ */
