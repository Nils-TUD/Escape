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

#include <sys/common.h>
#include <gui/scrollpane.h>
#include <gui/panel.h>
#include <iomanip>

using namespace std;

namespace gui {
	const gsize_t ScrollPane::BAR_SIZE			= 20;
	const gsize_t ScrollPane::MIN_SIZE			= BAR_SIZE + 40;
	const gsize_t ScrollPane::MIN_BAR_SIZE		= 10;
	const gsize_t ScrollPane::SCROLL_FACTOR		= 100;

	bool ScrollPane::resizeTo(const Size &size) {
		bool res = false;
		// ensure that we reach the minimum size
		Size rsize = maxsize(Size(MIN_SIZE,MIN_SIZE),size);
		res |= Control::resizeTo(rsize);

		// determine what size the control will actually use, if it gets visiblex X visibley.
		Size visible = rsize - Size(BAR_SIZE,BAR_SIZE);
		res |= _ctrl->resizeTo(_ctrl->getUsedSize(visible));

		// ensure that the position of the control is in the allowed range; of course, this changes
		// as soon as the size of the scrollpane or the control changes
		gpos_t minX = visible.width - _ctrl->getSize().width;
		gpos_t minY = visible.height - _ctrl->getSize().height;
		Pos newPos = minpos(Pos(0,0),maxpos(Pos(minX,minY),_ctrl->getPos()));
		if(_ctrl->getPos() != newPos)
			res |= _ctrl->moveTo(newPos);
		return res;
	}

	void ScrollPane::scrollBy(int x,int y,bool update) {
		Graphics *g = getGraphics();
		Size visible = getVisible();
		Pos cpos = _ctrl->getPos();
		if(x != 0 && visible.width != 0) {
			int minX = visible.width - _ctrl->getSize().width;
			// scroll left, i.e. move content right
			if(x < 0 && cpos.x < 0) {
				x = max(cpos.x,x);
				_ctrl->moveTo(Pos(cpos.x - x,cpos.y));
				if(-x >= (int)visible.width)
					repaint(update);
				else {
					// move cols right and repaint the first few cols
					g->moveCols(0,0,visible.width + x,visible.height,x);
					repaintRect(Pos(0,0),Size(-x,visible.height),update);
					makeDirty(true);
					repaintRect(Pos(0,visible.height),Size(getSize().width,BAR_SIZE),update);
				}
			}
			// scroll right, i.e. move content left
			else if(x > 0 && cpos.x > minX) {
				x = min(cpos.x - minX,x);
				_ctrl->moveTo(Pos(cpos.x - x,cpos.y));
				if(x >= (int)visible.width)
					repaint(update);
				else {
					// move cols left and repaint the last few cols
					g->moveCols(x,0,(gsize_t)(visible.width - x),visible.height,x);
					repaintRect(Pos(visible.width - x,0),Size(x,visible.height),update);
					makeDirty(true);
					repaintRect(Pos(0,visible.height),Size(getSize().width,BAR_SIZE),update);
				}
			}
		}
		else if(y != 0 && visible.height != 0) {
			int minY = visible.height - _ctrl->getSize().height;
			// scroll up, i.e. move content down
			if(y < 0 && cpos.y < 0) {
				y = max(cpos.y,y);
				_ctrl->moveTo(Pos(cpos.x,cpos.y - y));
				if(-y >= (int)visible.height)
					repaint(update);
				else {
					// move rows down and repaint the first few rows
					g->moveRows(0,0,visible.width,visible.height + y,y);
					repaintRect(Pos(0,0),Size(visible.width,-y));
					makeDirty(true);
					repaintRect(Pos(visible.width,0),Size(BAR_SIZE,getSize().height));
				}
			}
			// scroll down, i.e. move content up
			else if(y > 0 && cpos.y > minY) {
				y = min(cpos.y - minY,y);
				_ctrl->moveTo(Pos(cpos.x,cpos.y - y));
				if(y >= (int)visible.height)
					repaint(update);
				else {
					// move rows up and repaint the last few rows
					g->moveRows(0,y,visible.width,visible.height - y,y);
					repaintRect(Pos(0,visible.height - y),Size(visible.width,y),update);
					makeDirty(true);
					repaintRect(Pos(visible.width,0),Size(BAR_SIZE,getSize().height),update);
				}
			}
		}
	}

	void ScrollPane::onMouseMoved(const MouseEvent &e) {
		if(_focus & (FOCUS_HORSB | FOCUS_VERTSB)) {
			bool moving = _move.x || _move.y;
			_move.x += (_focus & FOCUS_HORSB) ? e.getXMovement() : 0;
			_move.y += (_focus & FOCUS_VERTSB) ? e.getYMovement() : 0;
			if(!moving) {
				Application::getInstance()->executeIn(SCROLL_TIMEOUT,
					make_memfun(this,&ScrollPane::performScroll));
			}
		}
	}
	void ScrollPane::onMouseReleased(A_UNUSED const MouseEvent &e) {
		_focus &= ~(FOCUS_HORSB | FOCUS_VERTSB);
		setFocus(_ctrl.get());
	}
	void ScrollPane::onMousePressed(const MouseEvent &e) {
		Size size = getSize();
		Pos epos = e.getPos();
		Pos cpos = _ctrl->getPos();
		if(epos.y >= (gpos_t)(size.height - BAR_SIZE)) {
			gsize_t visiblex = size.width - BAR_SIZE;
			if(epos.x < (gpos_t)visiblex) {
				gsize_t ctrlw = _ctrl->getSize().width;
				gpos_t barPos = getBarPos(ctrlw,visiblex,cpos.x);
				gsize_t barSize = getBarSize(ctrlw,visiblex);
				_focus = FOCUS_HORSB;
				if(epos.x < barPos)
					scrollRelatively(-barSize,0);
				else if(epos.x >= (gpos_t)(barPos + barSize))
					scrollRelatively(barSize,0);
			}
		}
		else if(epos.x >= (gpos_t)(size.width - BAR_SIZE)) {
			gsize_t ctrlh = _ctrl->getSize().height;
			gsize_t visibley = size.height - BAR_SIZE;
			gpos_t barPos = getBarPos(ctrlh,visibley,cpos.y);
			gsize_t barSize = getBarSize(ctrlh,visibley);
			_focus = FOCUS_VERTSB;
			if(epos.y < barPos)
				scrollRelatively(0,-barSize);
			else if(epos.y >= (gpos_t)(barPos + barSize))
				scrollRelatively(0,barSize);
		}
		else {
			MouseEvent ce(e.getType(),e.getXMovement(),e.getYMovement(),e.getWheelMovement(),
					  epos - cpos,e.getButtonMask());
			_ctrl->onMousePressed(ce);
			_focus = FOCUS_CTRL;
		}
	}
	void ScrollPane::onMouseWheel(const MouseEvent &e) {
		gsize_t visibley = getSize().height - BAR_SIZE;
		if(getBarSize(_ctrl->getSize().height,visibley) < visibley)
			scrollBy(0,e.getWheelMovement() * SCROLL_FACTOR);
		else
			scrollBy(e.getWheelMovement() * SCROLL_FACTOR,0);
	}

	void ScrollPane::paint(Graphics &g) {
		Size visible = getVisible();

		// only paint the background, if the control doesn't use up all space
		if(_ctrl->getSize().width < visible.width || _ctrl->getSize().height < visible.height) {
			g.setColor(getTheme().getColor(Theme::CTRL_BACKGROUND));
			g.fillRect(Pos(0,0),visible);
		}

		_ctrl->makeDirty(true);
		if(_update.empty()) {
			Size size = visible - Size(_ctrl->getPos());
			_ctrl->repaintRect(Pos(0,0) - _ctrl->getPos(),size,false);
		}
		else {
			Size rsize = minsize(_update.getSize(),visible);
			_ctrl->repaintRect(_update.getPos() - _ctrl->getPos(),rsize,false);
		}
		paintBars(g);
	}

	void ScrollPane::paintBars(Graphics &g) {
		Size ctrlSize = _ctrl->getSize();
		Size visible = getVisible();

		// horizontal scrollbar
		gpos_t ybarPos = visible.height;
		g.setColor(getTheme().getColor(Theme::CTRL_LIGHTBORDER));
		g.drawRect(0,ybarPos,visible.width,BAR_SIZE);
		g.setColor(getTheme().getColor(Theme::CTRL_LIGHTBACK));
		g.fillRect(1,ybarPos + 1,visible.width - 2,BAR_SIZE - 2);
		if(ctrlSize.width > visible.width) {
			g.setColor(getTheme().getColor(Theme::CTRL_DARKBACK));
			g.fillRect(getBarPos(ctrlSize.width,visible.width,_ctrl->getPos().x) + 2,
					ybarPos + 2,getBarSize(ctrlSize.width,visible.width) - 3,BAR_SIZE - 4);
		}

		// vertical scrollbar
		gpos_t xbarPos = visible.width;
		g.setColor(getTheme().getColor(Theme::CTRL_LIGHTBORDER));
		g.drawRect(xbarPos,0,BAR_SIZE,visible.height);
		g.setColor(getTheme().getColor(Theme::CTRL_LIGHTBACK));
		g.fillRect(xbarPos + 1,1,BAR_SIZE - 2,visible.height - 2);
		if(ctrlSize.height > visible.height) {
			g.setColor(getTheme().getColor(Theme::CTRL_DARKBACK));
			g.fillRect(xbarPos + 2,getBarPos(ctrlSize.height,visible.height,_ctrl->getPos().y) + 2,
					BAR_SIZE - 4,getBarSize(ctrlSize.height,visible.height) - 3);
		}

		// corner
		g.setColor(getTheme().getColor(Theme::CTRL_BACKGROUND));
		g.fillRect(xbarPos,ybarPos,BAR_SIZE,BAR_SIZE);
	}

	void ScrollPane::paintRect(Graphics &g,const Pos &pos,const Size &size) {
		_update = Rectangle(pos,size);
		UIElement::paintRect(g,pos,size);
		_update = Rectangle();
	}

	gpos_t ScrollPane::getBarPos(gsize_t ctrlSize,gsize_t viewable,gpos_t ctrlPos) {
		if(ctrlPos == 0 || ctrlSize == 0)
			return 0;
		return (gpos_t)(viewable / ((double)ctrlSize / -ctrlPos));
	}

	gsize_t ScrollPane::getBarSize(gsize_t ctrlSize,gsize_t viewable) {
		if(viewable == 0 || ctrlSize == 0)
			return 0;
		return max(MIN_BAR_SIZE,(gsize_t)(viewable / ((double)ctrlSize / viewable)));
	}
}
