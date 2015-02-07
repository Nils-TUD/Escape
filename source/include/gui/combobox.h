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

#include <gui/control.h>
#include <gui/popupwindow.h>
#include <sys/common.h>
#include <string>
#include <vector>

namespace gui {
	class ComboBox : public Control {
		friend class ItemWindow;

	private:
		static const gsize_t BTN_SIZE	= 20;
		static const gsize_t ARROW_PAD	= 8;
		static const gsize_t MAX_WIDTH	= 200;

		class ItemWindow : public PopupWindow {
			friend class ComboBox;

		private:
			static const gsize_t SELPAD		= 2;

		public:
			ItemWindow(ComboBox *cb,const Pos &pos,const Size &size)
				: PopupWindow(pos,size), _cb(cb), _highlighted(cb->_selected) {
			}

			virtual void onKeyPressed(const KeyEvent &e);
			virtual void onKeyReleased(const KeyEvent &e);
			virtual void onMouseMoved(const MouseEvent &e);
			virtual void onMouseReleased(const MouseEvent &e);
			void close(const Pos &pos);

		protected:
			virtual void paint(Graphics &g);

		private:
			void closeImpl();
			int getItemAt(const Pos &pos);

		private:
			ComboBox *_cb;
			int _highlighted;
		};

	public:
		typedef Sender<UIElement&> onchange_type;

		ComboBox()
			: Control(), _selected(-1), _pressed(false), _focused(false), _changed(), _win(), _items() {
		}
		ComboBox(const Pos &pos,const Size &size)
			: Control(pos,size), _selected(-1), _pressed(false), _focused(false), _changed(), _win(), _items() {
		}

		onchange_type &changed() {
			return _changed;
		}

		void addItem(const std::string &s) {
			_items.push_back(s);
			makeDirty(true);
		}
		const std::string *getSelectedItem() const {
			if(_selected == -1)
				return NULL;
			return &_items[_selected];
		}
		int getSelectedIndex() const {
			return _selected;
		}
		void setSelectedIndex(int index) {
			if(index >= 0 && index < (int)_items.size())
				doSetIndex(index);
			else
				doSetIndex(-1);
			makeDirty(true);
		}

		virtual void onFocusGained();
		virtual void onFocusLost();
		virtual void onKeyPressed(const KeyEvent &e);
		virtual void onKeyReleased(const KeyEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);

		virtual void print(esc::OStream &os, bool rec = true, size_t indent = 0) const {
			UIElement::print(os, rec, indent);
			os << " selected='" << (_selected != -1 ? _items[_selected] : "---") << "'";
		}

	protected:
		virtual Size getPrefSize() const;
		void removeWindow() {
			Application::getInstance()->removeWindow(_win);
			_win.reset();
		}
		void doSetIndex(int index) {
			bool diff = _selected != index;
			_selected = index;
			if(diff)
				_changed.send(*this);
		}
		void setPressed(bool pressed) {
			_pressed = pressed;
			makeDirty(true);
			repaint();
		}
		void setFocused(bool focused) {
			_focused = focused;
			makeDirty(true);
			repaint();
		}
		virtual void paint(Graphics &g);

	private:
		void showPopup();

		int _selected;
		bool _pressed;
		bool _focused;
		onchange_type _changed;
		std::shared_ptr<ItemWindow> _win;
		std::vector<std::string> _items;
	};
}
