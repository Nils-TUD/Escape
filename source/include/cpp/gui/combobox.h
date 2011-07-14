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

#ifndef COMBOBOX_H_
#define COMBOBOX_H_

#include <esc/common.h>
#include <gui/control.h>
#include <gui/graphics.h>
#include <gui/popupwindow.h>
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
			ItemWindow(ComboBox *cb,gpos_t x,gpos_t y,gsize_t width,gsize_t height)
				: PopupWindow(x,y,width,height), _cb(cb), _highlighted(cb->_selected) {
			};
			virtual ~ItemWindow() {
			};

			void onMouseMoved(const MouseEvent &e);
			void onMouseReleased(const MouseEvent &e);
			void close(gpos_t x,gpos_t y);
			void paint(Graphics &g);

		private:
			// no cloning
			ItemWindow(const ItemWindow &w);
			ItemWindow &operator=(const ItemWindow &w);

			void closeImpl();
			int getItemAt(gpos_t x,gpos_t y);

		private:
			ComboBox *_cb;
			int _highlighted;
		};

	public:
		ComboBox()
			: Control(), _items(vector<string>()), _selected(-1),
				_pressed(false), _win(NULL) {
		};
		ComboBox(gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: Control(x,y,width,height), _items(vector<string>()), _selected(-1),
				_pressed(false), _win(NULL) {
		};
		virtual ~ComboBox() {
			delete _win;
		};

		inline void addItem(const string &s) {
			_items.push_back(s);
		};
		inline int getSelectedIndex() const {
			return _selected;
		};
		inline void setSelectedIndex(int index) {
			if(index >= 0 && index < (int)_items.size())
				_selected = index;
			else
				_selected = -1;
			repaint();
		};

		virtual gsize_t getMinWidth() const;
		virtual gsize_t getMinHeight() const;
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void paint(Graphics &g);

	private:
		vector<string> _items;
		int _selected;
		bool _pressed;
		ItemWindow *_win;
	};
}

#endif /* COMBOBOX_H_ */
