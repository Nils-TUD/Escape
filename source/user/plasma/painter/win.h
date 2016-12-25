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

#include <gui/control.h>
#include <gui/window.h>
#include <stdlib.h>

#include "../painter.h"

extern bool handleKey(int keycode);

class WinPainter : public Painter {
	class PlasmaControl : public gui::Control {
	public:
		explicit PlasmaControl(WinPainter *_painter)
			: gui::Control(), add(), factor(1), time(0), painter(_painter) {
		}

		virtual gui::Size getPrefSize() const {
			// not used
			return gui::Size(100,50);
		}

		virtual void paint(gui::Graphics &) {
			painter->paintPlasma(add,factor,time);
		}

		RGB add;
		size_t factor;
		float time;
		WinPainter *painter;
	};

public:
	explicit WinPainter(gsize_t width,gsize_t height) : Painter(), painting(false), plasma() {
		using namespace gui;

		this->width = width;
		this->height = height;

		int tid;
		if((tid = startthread(guiThread,this)) < 0)
			error("Unable to start thread");
	}

	virtual void paint(const RGB &add,size_t factor,float time) {
		if(!plasma || !gui::Application::getInstance() || painting)
			return;

		painting = true;
		plasma->add = add;
		plasma->factor = factor;
		plasma->time = time;
		gui::Application::getInstance()->executeLater(std::make_lambda([this] {
			plasma->makeDirty(true);
			plasma->repaint();
			painting = false;
		}));
	}

private:
	virtual void set(const RGB &add,gpos_t x,gpos_t y,size_t factor,float val) {
		gui::Graphics *g = plasma->getGraphics();
		g->setColor(gui::Color(
			(int)(255 * (.5f + .5f * fastsin(val + add.r))),
			(int)(255 * (.5f + .5f * fastsin(val + add.g))),
			(int)(255 * (.5f + .5f * fastsin(val + add.b)))
		));

		gui::Pos base(x * factor,y * factor);
		for(size_t j = 0; j < factor; ++j) {
			gui::Pos pos(base.x,base.y + j);
			for(size_t i = 0; i < factor; ++i) {
				g->setPixel(pos);
				pos.x += 1;
			}
		}
	}

	void onKeyPressed(gui::UIElement &,const gui::KeyEvent &e) {
		handleKey(e.getKeyCode());
		if(e.getKeyCode() == VK_Q)
			gui::Application::getInstance()->exit();
	}

	void onResize(gui::Window &) {
		resize(plasma->getSize().width,plasma->getSize().height);
		plasma->makeDirty(true);
		plasma->repaint();
	}

	static int guiThread(void *arg) {
		using namespace gui;

		WinPainter *painter = reinterpret_cast<WinPainter*>(arg);
		int res = EXIT_FAILURE;

		try {
			Application *app = Application::create();
			std::shared_ptr<Window> win = make_control<Window>(
				"Plasma",Pos(100,50),Size(painter->width,painter->height));
			std::shared_ptr<Panel> root = win->getRootPanel();
			root->setLayout(make_layout<BorderLayout>());
			root->getTheme().setPadding(0);

			painter->plasma = make_control<PlasmaControl>(painter);
			root->add(painter->plasma,BorderLayout::CENTER);

			win->keyPressed().subscribe(mem_recv(painter,&WinPainter::onKeyPressed));
			win->resized().subscribe(mem_recv(painter,&WinPainter::onResize));

			painter->resize(painter->plasma->getSize().width,painter->plasma->getSize().height);

			win->show(false);
			app->addWindow(win);

			/* start */
			painter->resize(painter->plasma->getSize().width,painter->plasma->getSize().height);

			res = app->run();
		}
		catch(const std::exception &e) {
			printe(e.what());
		}

		handleKey(VK_Q);
		return res;
	}

	bool painting;
	std::shared_ptr<PlasmaControl> plasma;
};
