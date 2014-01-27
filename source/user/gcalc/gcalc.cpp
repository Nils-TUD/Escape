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

#include <esc/common.h>
#include <gui/layout/gridlayout.h>
#include <gui/application.h>
#include <gui/window.h>
#include <gui/button.h>
#include <gui/editable.h>
#include <gui/combobox.h>
#include <gui/scrollpane.h>
#include <stdlib.h>
#include <sstream>
#include "gcalc.h"

using namespace gui;
using namespace std;

static const Size BTN_SIZE(30,30);

EXTERN_C void yylex_destroy(void);
EXTERN_C void yyerror(char const *s);
EXTERN_C int yyparse(void);

const char *parse_text = nullptr;
static shared_ptr<Window> win;
static shared_ptr<Editable> textfield;

void yyerror(char const *s) {
	cerr << "Error: " << s << endl;
}

void callback(double result) {
	ostringstream ostr;
	ostr << result;
	textfield->setText(ostr.str());
	textfield->repaint();
}

static void parse() {
	char *text = strdup(textfield->getText().c_str());
	parse_text = text;
	// we need to reset the scanner if an error happened
	if(yyparse() != 0)
		yylex_destroy();
	free(text);
	parse_text = nullptr;
}

static void onButtonClick(char c,UIElement&) {
	textfield->insertAtCursor(c);
	textfield->repaint();
}

static void onClearButtonClick(UIElement&) {
	textfield->setText("");
	textfield->repaint();
}

static void onSubmitButtonClick(UIElement&) {
	parse();
}

static void keyPressed(UIElement &,const KeyEvent &e) {
	if(e.getKeyCode() == VK_ENTER)
		parse();
	if(win->getFocus() == textfield.get())
		return;

	if(e.getKeyCode() == VK_BACKSP)
		textfield->removeLast();
	else if(e.getKeyCode() == VK_DELETE)
		textfield->removeNext();
	else {
		switch(e.getCharacter()) {
			case '0' ... '9':
			case '.':
			case '%':
			case '+':
			case '*':
			case '-':
			case '/':
			case '(':
			case ')':
				textfield->insertAtCursor(e.getCharacter());
				break;
		}
	}
	win->requestFocus(textfield.get());
	textfield->repaint();
}

int main() {
	Application *app = Application::create();
	win = make_control<Window>("Calculator",Pos(250,250));
	win->keyPressed().subscribe(func_recv(keyPressed));
	shared_ptr<Panel> root = win->getRootPanel();
	root->getTheme().setPadding(2);
	root->setLayout(make_layout<BorderLayout>(2));

	shared_ptr<Panel> grid = make_control<Panel>(make_layout<GridLayout>(5,4));
	root->add(grid,BorderLayout::CENTER);

	textfield = make_control<Editable>();
	root->add(textfield,BorderLayout::NORTH);

	struct sCalcButton {
		char c;
		GridPos pos;
	} buttons[] = {
		{'7', GridPos(0,0)},
		{'8', GridPos(1,0)},
		{'9', GridPos(2,0)},
		{'4', GridPos(0,1)},
		{'5', GridPos(1,1)},
		{'6', GridPos(2,1)},
		{'1', GridPos(0,2)},
		{'2', GridPos(1,2)},
		{'3', GridPos(2,2)},
		{'0', GridPos(0,3)},
		{'.', GridPos(1,3)},
		{'%', GridPos(2,3)},
		{'+', GridPos(3,0)},
		{'*', GridPos(3,1)},
		{'-', GridPos(3,2)},
		{'/', GridPos(3,3)},
		{'(', GridPos(4,0)},
		{')', GridPos(4,1)},
	};
	for(size_t i = 0; i < ARRAY_SIZE(buttons); ++i) {
		char name[] = {buttons[i].c,'\0'};
		shared_ptr<Button> b = make_control<Button>(name,Pos(0,0),BTN_SIZE);
		b->clicked().subscribe(bind1_func_recv(buttons[i].c,onButtonClick));
		grid->add(b,buttons[i].pos);
	}

	{
		shared_ptr<Button> b = make_control<Button>("C",Pos(0,0),BTN_SIZE);
		b->clicked().subscribe(func_recv(onClearButtonClick));
		grid->add(b,GridPos(4,2));
	}

	{
		shared_ptr<Button> b = make_control<Button>("=",Pos(0,0),BTN_SIZE);
		b->clicked().subscribe(func_recv(onSubmitButtonClick));
		grid->add(b,GridPos(4,3));
	}

	win->show(true);
	app->addWindow(win);
	return app->run();
}
