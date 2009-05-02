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

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/gui/button.h>
#include <esc/gui/control.h>

namespace esc {
	namespace gui {
		void Button::paint() {
			Control::paint();

			_g->setColor(bgColor);
			_g->fillRect(1,1,getWidth() - 2,getHeight() - 2);

			_g->setColor(lightBorderColor);
			_g->drawLine(0,0,getWidth() - 1,0);
			_g->drawLine(0,0,0,getHeight() - 1);

			_g->setColor(darkBorderColor);
			_g->drawLine(getWidth() - 1,0,getWidth() - 1,getHeight() - 1);
			_g->drawLine(0,getHeight() - 1,getWidth() - 1,getHeight() - 1);

			_g->setColor(fgColor);
			_g->drawString((getWidth() - _g->getFont().getStringWidth(_text)) / 2,
					(getHeight() - _g->getFont().getHeight()) / 2,_text);

			_g->update();
		}
	}
}
