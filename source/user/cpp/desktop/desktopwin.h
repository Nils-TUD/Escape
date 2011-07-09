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

#ifndef DESKTOPWIN_H_
#define DESKTOPWIN_H_

#include <esc/common.h>
#include <gui/color.h>
#include <gui/window.h>
#include <gui/image.h>
#include <gui/imagebutton.h>
#include <gui/actionlistener.h>
#include <map>

class Shortcut {
	friend class DesktopWin;

public:
	Shortcut(const std::string& icon,const std::string& app)
		: _icon(icon), _app(app), _btn(NULL) {
	};
	Shortcut(const Shortcut &w)
		: _icon(w._icon), _app(w._app), _btn(new gui::ImageButton(*w._btn)) {
	};
	~Shortcut() {
	};

	Shortcut &operator=(const Shortcut &w) {
		if(this == &w)
			return *this;
		this->_icon = w._icon;
		this->_app = w._app;
		this->_btn = new gui::ImageButton(*w._btn);
		return *this;
	};

	inline const string& getIcon() const {
		return _icon;
	};
	inline const string& getApp() const {
		return _app;
	};

private:
	inline gui::ImageButton *getButton() const {
		return _btn;
	};
	inline void setButton(gui::ImageButton *btn) {
		_btn = btn;
	};

private:
	std::string _icon;
	std::string _app;
	gui::ImageButton *_btn;
};

class DesktopWin : public gui::Window, public gui::ActionListener {
public:
	static const gsize_t PADDING;
	static const gsize_t ICON_SIZE;
	static const gui::Color BGCOLOR;

public:
	DesktopWin(gsize_t width,gsize_t height)
		: gui::Window("",0,0,width,height,STYLE_DESKTOP),
		  	  _shortcuts(map<gui::ImageButton*,Shortcut*>()) {
		// TODO thats not good. perhaps provide a opportunity to disable the header?
		_header.resizeTo(_header.getWidth(),0);
		_body.moveTo(0,0);
	};
	virtual ~DesktopWin() {
	};

	inline void addShortcut(Shortcut* sc) {
		// do that first for exception-safety
		gui::Image *img = gui::Image::loadImage(sc->getIcon());
		gui::ImageButton *btn = new gui::ImageButton(img,
				PADDING,PADDING + _shortcuts.size() * (ICON_SIZE + PADDING),
				img->getWidth() + 2,img->getHeight() + 2);
		sc->setButton(btn);
		btn->addListener(this);
		_shortcuts[btn] = sc;
		getRootPanel().add(*btn);
		repaint();
	};

	virtual void actionPerformed(UIElement& el);
	virtual void paint(gui::Graphics &g);

	// no cloning
private:
	DesktopWin(const DesktopWin &w);
	DesktopWin &operator=(const DesktopWin &w);

private:
	map<gui::ImageButton*,Shortcut*> _shortcuts;
};

#endif /* DESKTOPWIN_H_ */
