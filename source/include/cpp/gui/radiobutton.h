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
#include <gui/toggle.h>

namespace gui {
	class RadioGroup;

	class RadioButton : public Toggle {
		friend class RadioGroup;

		static const gsize_t TEXT_PADDING	= 6;

	public:
		explicit RadioButton(RadioGroup &group,const std::string &text);
		virtual ~RadioButton();

	protected:
		virtual void paint(Graphics &g);

	private:
		virtual Size getPrefSize() const;
		virtual bool doSetSelected(bool selected);
		void select(bool selected) {
			if(Toggle::doSetSelected(selected))
				repaint();
		}

		RadioGroup &_group;
	};
}
