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
#include <gui/graphics/color.h>
#include <gui/image/image.h>
#include <gui/layout/borderlayout.h>
#include <gui/layout/flowlayout.h>
#include <gui/window.h>
#include <gui/imagebutton.h>
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

	Shortcut &operator=(const Shortcut &w) {
		if(this == &w)
			return *this;
		this->_icon = w._icon;
		this->_app = w._app;
		this->_btn = new gui::ImageButton(*w._btn);
		return *this;
	};

	const string& getIcon() const {
		return _icon;
	};
	const string& getApp() const {
		return _app;
	};

private:
	gui::ImageButton *getButton() const {
		return _btn;
	};
	void setButton(gui::ImageButton *btn) {
		_btn = btn;
	};

private:
	std::string _icon;
	std::string _app;
	gui::ImageButton *_btn;
};

class DesktopWin : public gui::Window {
	class WinButton : public gui::Button {
	public:
		WinButton(DesktopWin *inst,const string &title)
			: Button(title), _sub(clicked(),mem_recv(inst,&DesktopWin::onIconClick)) {
		};

	private:
		onclick_type::subscr_type _sub;
	};

	typedef map<gwinid_t,WinButton*> winmap_type;
	typedef map<gui::ImageButton*,Shortcut*> shortcutmap_type;

public:
	static const gsize_t PADDING;
	static const gsize_t ICON_SIZE;
	static const gui::Color BGCOLOR;
	static const gui::Color ACTIVE_COLOR;
	static const gsize_t TASKBAR_HEIGHT;

public:
	DesktopWin(const gui::Size &size);

	void addShortcut(Shortcut* sc) {
		// do that first for exception-safety
		gui::Image *img = gui::Image::loadImage(sc->getIcon());
		gui::ImageButton *btn = new gui::ImageButton(img,
				gui::Pos(PADDING,PADDING + _shortcuts.size() * (ICON_SIZE + PADDING)),
				img->getSize() + gui::Size(2,2));
		sc->setButton(btn);
		btn->clicked().subscribe(mem_recv(this,&DesktopWin::onIconClick));
		_shortcuts[btn] = sc;
		_iconPanel->add(btn);
		_iconPanel->repaint();
	};

	// no cloning
private:
	DesktopWin(const DesktopWin &w);
	DesktopWin &operator=(const DesktopWin &w);

	void onWindowCreated(gwinid_t id,const std::string& title);
	void onWindowActive(gwinid_t id);
	void onWindowDestroyed(gwinid_t id);

	void init();
	void onIconClick(UIElement& el);

private:
	gui::Panel *_winPanel;
	gui::Panel *_iconPanel;
	WinButton *_active;
	winmap_type _windows;
	shortcutmap_type _shortcuts;
};
