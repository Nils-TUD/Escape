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

#include <sys/common.h>
#include <gui/enums.h>
#include <gui/splitter.h>
#include <gui/panel.h>

namespace gui {
	class SplitPanel : public Panel {
		static const uint TIMEOUT	= 20;

	public:
		explicit SplitPanel(Orientation orientation)
			: Panel(), _orientation(orientation), _position(-1), _moving(false), _refreshing(false) {
			init();
		}
		explicit SplitPanel(Orientation orientation,const Pos &pos,const Size &size)
			: Panel(pos,size), _orientation(orientation), _position(-1), _moving(false),
			  _refreshing(false) {
			init();
		}

		virtual Size getUsedSize(const Size &avail) const;

		void add(std::shared_ptr<Control> ctrl) {
			if(_controls.size() < 3)
				Panel::add(ctrl,0);
			else
				throw std::logic_error("Only two controls are supported");
		}

		virtual void onMouseMoved(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void onMousePressed(const MouseEvent &e);

		virtual bool layout();

		virtual bool resizeTo(const Size &size);

	protected:
		virtual void onFocusGained() {
			if(!_moving)
				Panel::onFocusGained();
		}

		virtual Control *getFocus() {
			if(!_moving)
				return Panel::getFocus();
			return this;
		}

	private:
		void init() {
			_controls.reserve(3);
			Panel::add(make_control<Splitter>(_orientation),0);
		}
		void refresh();
		Size combine(Size a,Size b,Size splitter) const;
		virtual Size getPrefSize() const;

		Orientation _orientation;
		double _position;
		bool _moving;
		bool _refreshing;
	};
}
