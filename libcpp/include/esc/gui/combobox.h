/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/gui/common.h>
#include <esc/gui/control.h>
#include <esc/gui/graphics.h>
#include <esc/gui/popupwindow.h>

namespace esc {
	namespace gui {
		class ComboBox : public Control {
			friend class ItemWindow;

		private:
			static const Color BORDERCOLOR;
			static const Color ITEM_BGCOLOR;
			static const Color ITEM_FGCOLOR;
			static const Color BTN_BGCOLOR;
			static const Color BTN_ARROWCOLOR;

			class ItemWindow : public PopupWindow {
				friend class ComboBox;

			private:
				static const Color SEL_BGCOLOR;
				static const Color SEL_FGCOLOR;
				static const int PADDING = 4;

			public:
				ItemWindow(ComboBox *cb,tCoord x,tCoord y,tSize width,tSize height)
					: PopupWindow(x,y,width,height), _cb(cb), _highlighted(cb->_selected) {
				};
				virtual ~ItemWindow() {
				};

				void onMouseMoved(const MouseEvent &e);
				void onMouseReleased(const MouseEvent &e);
				void close(tCoord x,tCoord y);
				void paint(Graphics &g);

			private:
				// no copying
				ItemWindow(const ItemWindow &w);
				ItemWindow &operator=(const ItemWindow &w);

				void closeImpl();
				s32 getItemAt(tCoord x,tCoord y);

			private:
				ComboBox *_cb;
				s32 _highlighted;
			};

		public:
			ComboBox(tCoord x,tCoord y,tSize width,tSize height)
				: Control(x,y,width,height), _items(Vector<String>()), _selected(-1),
					_pressed(false), _win(NULL) {
			};
			ComboBox(const ComboBox &cb)
				: Control(cb), _items(Vector<String>(cb._items)), _selected(cb._selected),
					_pressed(false), _win(NULL) {
			};
			virtual ~ComboBox() {
				delete _win;
			};

			ComboBox &operator=(const ComboBox &cb) {
				// ignore self-assignments
				if(this == &cb)
					return *this;
				Control::operator=(cb);
				_items = cb._items;
				_pressed = cb._pressed;
				_win = cb._win;
				return *this;
			};

			inline void addItem(const String &s) {
				_items.add(s);
			};
			inline s32 getSelectedIndex() const {
				return _selected;
			};
			inline void setSelectedIndex(s32 index) {
				if(index >= 0 && index < (s32)_items.size())
					_selected = index;
				else
					_selected = -1;
				repaint();
			};

			virtual void onMousePressed(const MouseEvent &e);
			virtual void onMouseReleased(const MouseEvent &e);
			virtual void paint(Graphics &g);

		private:
			Vector<String> _items;
			s32 _selected;
			bool _pressed;
			ItemWindow *_win;
		};
	}
}

#endif /* COMBOBOX_H_ */
