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

#include <esc/common.h>
#include <gui/scrollpane.h>
#include <gui/panel.h>

namespace gui {
	const Color ScrollPane::BGCOLOR = Color(0x88,0x88,0x88);
	const Color ScrollPane::BARBGCOLOR = Color(0x80,0x80,0x80);
	const Color ScrollPane::SEPCOLOR = Color(0x50,0x50,0x50);
	const Color ScrollPane::BARCOLOR = Color(0x60,0x60,0x60);

	void ScrollPane::resizeTo(gsize_t width,gsize_t height) {
		Control::resizeTo(width,height);
		_ctrl->resizeTo(_ctrl->getPreferredWidth(),_ctrl->getPreferredHeight());
		// ensure that the position of the control is in the allowed range; of course, this changes
		// as soon as the size of the scrollpane or the control changes
		gsize_t visiblex = getWidth() - BAR_SIZE;
		gsize_t visibley = getHeight() - BAR_SIZE;
		gpos_t minX = visiblex - _ctrl->getWidth();
		gpos_t minY = visibley - _ctrl->getHeight();
		gpos_t newX = min((gpos_t)0,max(minX,_ctrl->getX()));
		gpos_t newY = min((gpos_t)0,max(minY,_ctrl->getY()));
		if(_ctrl->getX() != newX || _ctrl->getY() != newY)
			_ctrl->moveTo(newX,newY);
	}

	void ScrollPane::onMouseMoved(const MouseEvent &e) {
		if((_focus & FOCUS_HORSB) && e.getXMovement() != 0) {
			gsize_t visible = getWidth() - BAR_SIZE;
			short x = _ctrl->getWidth() / (visible / e.getXMovement());
			int minX = visible - _ctrl->getWidth();
			if(x < 0 && _ctrl->getX() < 0) {
				_ctrl->moveTo(min(_ctrl->getX() - x,0),_ctrl->getY());
				repaint();
			}
			else if(x > 0 && _ctrl->getX() > minX) {
				_ctrl->moveTo(max(_ctrl->getX() - x,minX),_ctrl->getY());
				repaint();
			}
		}
		else if((_focus & FOCUS_VERTSB) && e.getYMovement() != 0) {
			gsize_t visible = getHeight() - BAR_SIZE;
			short y = _ctrl->getHeight() / (visible / e.getYMovement());
			int minY = visible - _ctrl->getHeight();
			if(y < 0 && _ctrl->getY() < 0) {
				_ctrl->moveTo(_ctrl->getX(),min(_ctrl->getY() - y,0));
				repaint();
			}
			else if(y > 0 && _ctrl->getY() > minY) {
				_ctrl->moveTo(_ctrl->getX(),max(_ctrl->getY() - y,minY));
				repaint();
			}
		}
	}
	void ScrollPane::onMouseReleased(const MouseEvent &e) {
		UNUSED(e);
		_focus &= ~(FOCUS_HORSB | FOCUS_VERTSB);
	}
	void ScrollPane::onMousePressed(const MouseEvent &e) {
		if(e.getY() >= getHeight() - BAR_SIZE)
			_focus = FOCUS_HORSB;
		else if(e.getX() >= getWidth() - BAR_SIZE)
			_focus = FOCUS_VERTSB;
		else {
			MouseEvent ce(e.getType(),e.getXMovement(),e.getYMovement(),
					e.getX() - _ctrl->getX(),e.getY() - _ctrl->getY(),e.getButtonMask());
			_ctrl->onMousePressed(ce);
			_focus = FOCUS_CTRL;
		}
	}

	void ScrollPane::paint(Graphics &g) {
		_ctrl->paint(*_ctrl->getGraphics());
		gsize_t ctrlw = _ctrl->getWidth();
		gsize_t ctrlh = _ctrl->getHeight();
		gsize_t visiblex = getWidth() - BAR_SIZE;
		gsize_t visibley = getHeight() - BAR_SIZE;

		// horizontal scrollbar
		gpos_t ybarPos = visibley;
		g.setColor(SEPCOLOR);
		g.drawRect(0,ybarPos,visiblex,BAR_SIZE);
		g.setColor(BARBGCOLOR);
		g.fillRect(1,ybarPos + 1,visiblex - 2,BAR_SIZE - 2);
		if(ctrlw > visiblex) {
			g.setColor(BARCOLOR);
			g.fillRect(getBarPos(ctrlw,visiblex,_ctrl->getX()) + 2,
					ybarPos + 2,getBarSize(ctrlw,visiblex) - 3,BAR_SIZE - 4);
		}

		// vertical scrollbar
		gpos_t xbarPos = visiblex;
		g.setColor(SEPCOLOR);
		g.drawRect(xbarPos,0,BAR_SIZE,visibley);
		g.setColor(BARBGCOLOR);
		g.fillRect(xbarPos + 1,1,BAR_SIZE - 2,visibley - 2);
		if(ctrlh > visibley) {
			g.setColor(BARCOLOR);
			g.fillRect(xbarPos + 2,getBarPos(ctrlh,visibley,_ctrl->getY()) + 2,
					BAR_SIZE - 4,getBarSize(ctrlh,visibley) - 3);
		}

		// corner
		g.setColor(BGCOLOR);
		g.fillRect(xbarPos,ybarPos,BAR_SIZE,BAR_SIZE);
	}

	gpos_t ScrollPane::getBarPos(gsize_t ctrlSize,gsize_t viewable,gpos_t ctrlPos) {
		if(ctrlPos == 0 || ctrlSize == 0)
			return 0;
		return (gpos_t)(viewable / ((double)ctrlSize / -ctrlPos));
	}

	gsize_t ScrollPane::getBarSize(gsize_t ctrlSize,gsize_t viewable) {
		if(viewable == 0 || ctrlSize == 0)
			return 0;
		return (gsize_t)(viewable / ((double)ctrlSize / viewable));
	}
}
