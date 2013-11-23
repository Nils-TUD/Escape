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
#include <gui/toggle.h>

namespace gui {
	class Checkbox : public Toggle {
	private:
		static const gsize_t CROSS_PADDING	= 3;
		static const gsize_t TEXT_PADDING	= 4;

	public:
		Checkbox(const std::string &text) : Toggle(text) {
		}
		Checkbox(const std::string &text,const Pos &pos,const Size &size) : Toggle(text,pos,size) {
		}

	protected:
		virtual void paint(Graphics &g);

	private:
		virtual Size getPrefSize() const;
	};
}
