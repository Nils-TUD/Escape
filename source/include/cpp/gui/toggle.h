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
#include <gui/event/subscriber.h>
#include <gui/control.h>
#include <string>

namespace gui {
	class Toggle : public Control {
	public:
		typedef Sender<UIElement&> onchange_type;

		Toggle(const std::string &text)
			: Control(), _focused(false), _selected(false), _text(text), _changed() {
		}
		Toggle(const std::string &text,const Pos &pos,const Size &size)
			: Control(pos,size), _focused(false), _selected(false), _text(text), _changed() {
		}

		bool isSelected() const {
			return _selected;
		}
		void setSelected(bool selected) {
			doSetSelected(selected);
		}
		bool isFocused() const {
			return _focused;
		}
		const std::string &getText() const {
			return _text;
		}
		void setText(const std::string &text) {
			_text = text;
			makeDirty(true);
		}

		virtual void onFocusGained();
		virtual void onFocusLost();
		virtual void onKeyReleased(const KeyEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);

		onchange_type &changed() {
			return _changed;
		}

		virtual void print(std::ostream &os, bool rec = true, size_t indent = 0) const {
			UIElement::print(os, rec, indent);
			os << " selected=" << _selected << ", text='" << _text << "'";
		}

	protected:
		virtual bool doSetSelected(bool selected) {
			if(_selected != selected) {
				_selected = selected;
				_changed.send(*this);
				makeDirty(true);
				return true;
			}
			return false;
		}
		void setFocused(bool focused) {
			_focused = focused;
			makeDirty(true);
			repaint();
		}

	private:
		bool _focused;
		bool _selected;
		std::string _text;
		onchange_type _changed;
	};
}
