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
#include <gui/popupwindow.h>
#include <gui/control.h>
#include <string>
#include <vector>

namespace gui {
	class ComboBox : public Control {
		friend class ItemWindow;

	private:
		static const gsize_t BTN_SIZE	= 20;
		static const gsize_t ARROW_PAD	= 4;
		static const gsize_t MAX_WIDTH	= 200;

		class ItemWindow : public PopupWindow {
			friend class ComboBox;

		private:
			static const gsize_t SELPAD		= 2;

		public:
			ItemWindow(ComboBox *cb,const Pos &pos,const Size &size)
				: PopupWindow(pos,size), _cb(cb), _highlighted(cb->_selected) {
			}

			void onMouseMoved(const MouseEvent &e);
			void onMouseReleased(const MouseEvent &e);
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
		ComboBox()
			: Control(), _selected(-1), _pressed(false), _win(), _items() {
		}
		ComboBox(const Pos &pos,const Size &size)
			: Control(pos,size), _selected(-1), _pressed(false), _win(), _items() {
		}

		void addItem(const std::string &s) {
			_items.push_back(s);
		}
		int getSelectedIndex() const {
			return _selected;
		}
		void setSelectedIndex(int index) {
			if(index >= 0 && index < (int)_items.size())
				_selected = index;
			else
				_selected = -1;
			repaint();
		}

		virtual Size getPrefSize() const;
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);

		virtual void print(std::ostream &os, size_t indent = 0) const {
			UIElement::print(os, indent);
			os << " selected='" << (_selected != -1 ? _items[_selected] : "---") << "'";
		}

	protected:
		virtual void paint(Graphics &g);

	private:
		int _selected;
		bool _pressed;
		std::shared_ptr<ItemWindow> _win;
		std::vector<std::string> _items;
	};
}
