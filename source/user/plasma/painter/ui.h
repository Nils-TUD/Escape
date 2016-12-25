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

#include <esc/proto/ui.h>
#include <sys/esccodes.h>
#include <sys/thread.h>
#include <stdlib.h>

#include "../painter.h"

extern bool handleKey(int keycode);

class UIPainter : public Painter {
public:
	explicit UIPainter()
		: Painter(), ui(new esc::UI("/dev/uimng")), uiev(new esc::UIEvents(*ui)), fb(), mode() {
	}

	void start() {
		if(startthread(inputThread,this) < 0)
			error("Unable to start input thread");
	}

	virtual void paint(const RGB &add,size_t factor,float time) {
		paintPlasma(add,factor,time);
		ui->update(0,0,width,height);
		// we want to have a synchronous update
		ui->getMode();
	}

protected:
	static int inputThread(void *arg) {
		UIPainter *painter = reinterpret_cast<UIPainter*>(arg);
		/* read from uimanager and handle the keys */
		while(1) {
			esc::UIEvents::Event ev;
			*painter->uiev >> ev;
			if(ev.type != esc::UIEvents::Event::TYPE_KEYBOARD || (ev.d.keyb.modifier & STATE_BREAK))
				continue;

			if(!handleKey(ev.d.keyb.keycode))
				break;
		}
		return 0;
	}

	esc::UI *ui;
	esc::UIEvents *uiev;
	esc::FrameBuffer *fb;
	esc::Screen::Mode mode;
};
