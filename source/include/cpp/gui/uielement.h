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

#ifndef UIELEMENT_H_
#define UIELEMENT_H_

#include <esc/common.h>
#include <esc/messages.h>
#include <gui/graphics.h>
#include <gui/event.h>
#include <gui/mouselistener.h>
#include <gui/keylistener.h>
#include <vector>

namespace gui {
	class UIElement {
		// window and control need to access _g
		friend class Window;
		friend class Control;

	public:
		UIElement(gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: _g(NULL), _x(x), _y(y), _width(width), _height(height), _mlist(NULL),
			_klist(NULL) {
		};
		UIElement(const UIElement &e)
			: _g(NULL), _x(e._x), _y(e._y), _width(e._width), _height(e._height),
			_mlist(e._mlist), _klist(e._klist) {
		}
		virtual ~UIElement() {
			delete _mlist;
			delete _klist;
			delete _g;
		};
		UIElement &operator=(const UIElement &e);

		inline gpos_t getX() const {
			return _x;
		};
		inline gpos_t getY() const {
			return _y;
		};
		inline gsize_t getWidth() const {
			return _width;
		};
		inline gsize_t getHeight() const {
			return _height;
		};

		inline Graphics *getGraphics() const {
			return _g;
		};

		void addMouseListener(MouseListener *l);
		void removeMouseListener(MouseListener *l);

		void addKeyListener(KeyListener *l);
		void removeKeyListener(KeyListener *l);

		virtual void onMouseMoved(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onKeyPressed(const KeyEvent &e);
		virtual void onKeyReleased(const KeyEvent &e);

		virtual void repaint();
		virtual void paint(Graphics &g) = 0;
		void requestUpdate();

	protected:
		inline void setX(gpos_t x) {
			_x = x;
		};
		inline void setY(gpos_t y) {
			_y = y;
		};
		inline void setWidth(gsize_t width) {
			_width = width;
		};
		inline void setHeight(gsize_t height) {
			_height = height;
		};
		virtual gwinid_t getWindowId() const = 0;

	private:
		void notifyListener(const MouseEvent &e);
		void notifyListener(const KeyEvent &e);

	private:
		Graphics *_g;
		gpos_t _x;
		gpos_t _y;
		gsize_t _width;
		gsize_t _height;
		vector<MouseListener*> *_mlist;
		vector<KeyListener*> *_klist;
	};
}

#endif /* UIELEMENT_H_ */
