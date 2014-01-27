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
#include <gui/radiobutton.h>
#include <memory>
#include <vector>

namespace gui {
	class RadioGroup {
		friend class RadioButton;

	public:
		explicit RadioGroup() : _selected(-1), _btns() {
		}

		RadioButton *getSelected() {
			return _selected == -1 ? NULL : _btns[_selected];
		}
		ssize_t getSelectedIndex() const {
			return _selected;
		}
		void setSelected(RadioButton *btn) {
			std::vector<RadioButton*>::iterator it = std::find(_btns.begin(),_btns.end(),btn);
			if(it != _btns.end())
				setSelectedIndex(it - _btns.begin());
		}
		void setSelectedIndex(ssize_t idx) {
			ssize_t old = _selected;
			if(idx >= 0 && (size_t)idx < _btns.size())
				_selected = idx;
			else
				_selected = -1;
			if(old != _selected) {
				if(old != -1)
					_btns[old]->select(false);
				if(_selected != -1)
					_btns[_selected]->select(true);
			}
		}

	private:
		void add(RadioButton *btn) {
			_btns.push_back(btn);
		}
		void remove(RadioButton *btn) {
			if(_selected != -1 && _btns[_selected] == btn)
				setSelectedIndex(-1);
			_btns.erase_first(btn);
		}

		ssize_t _selected;
		std::vector<RadioButton*> _btns;
	};
}
