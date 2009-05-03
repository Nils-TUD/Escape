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
#include <esc/proc.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/gui/application.h>
#include <esc/gui/window.h>
#include <esc/gui/button.h>
#include <stdlib.h>

using namespace esc::gui;

int main(void) {
	Application *app = Application::getInstance();
	Window w1("Fenster 1",100,100,400,300);
	Window w2("Fenster 2",250,250,150,200);
	Window w3("Fenster 3",50,50,100,40);
	Window w4("Fenster 4",180,90,200,100);
	Button b("Click me!!",10,10,80,20);
	w1.add(b);
	return app->run();
}
