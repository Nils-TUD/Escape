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
#include <gui/borderlayout.h>
#include <gui/flowlayout.h>
#include <map>

using namespace gui;

class Shortcut {
	friend class DesktopWin;

public:
	Shortcut(const std::string& icon,const std::string& app)
		: _icon(icon), _app(app), _btn(NULL) {
	};
	Shortcut(const Shortcut &w)
		: _icon(w._icon), _app(w._app), _btn(new ImageButton(*w._btn)) {
	};
	~Shortcut() {
	};

	Shortcut &operator=(const Shortcut &w) {
		if(this == &w)
			return *this;
		this->_icon = w._icon;
		this->_app = w._app;
		this->_btn = new ImageButton(*w._btn);
		return *this;
	};

	inline const string& getIcon() const {
		return _icon;
	};
	inline const string& getApp() const {
		return _app;
	};

private:
	inline ImageButton *getButton() const {
		return _btn;
	};
	inline void setButton(ImageButton *btn) {
		_btn = btn;
	};

private:
	std::string _icon;
	std::string _app;
	ImageButton *_btn;
};

class DesktopWin : public Window, public ActionListener, public WindowListener {
public:
	static const gsize_t PADDING;
	static const gsize_t ICON_SIZE;
	static const Color BGCOLOR;
	static const Color ACTIVE_COLOR;
	static const gsize_t TASKBAR_HEIGHT;

public:
	DesktopWin(gsize_t width,gsize_t height);
	virtual ~DesktopWin() {
	};

	inline void addShortcut(Shortcut* sc) {
		// do that first for exception-safety
		Image *img = Image::loadImage(sc->getIcon());
		ImageButton *btn = new ImageButton(img,
				PADDING,PADDING + _shortcuts.size() * (ICON_SIZE + PADDING),
				img->getWidth() + 2,img->getHeight() + 2);
		sc->setButton(btn);
		btn->addListener(this);
		_shortcuts[btn] = sc;
		_iconPanel->add(btn);
		_iconPanel->repaint();
	};

	virtual void actionPerformed(UIElement& el);
	virtual void onWindowCreated(gwinid_t id,const std::string& title);
	virtual void onWindowActive(gwinid_t id);
	virtual void onWindowDestroyed(gwinid_t id);

	// no cloning
private:
	DesktopWin(const DesktopWin &w);
	DesktopWin &operator=(const DesktopWin &w);

	void init();

private:
	Panel *_winPanel;
	Panel *_iconPanel;
	Button *_active;
	map<gwinid_t,Button*> _windows;
	map<ImageButton*,Shortcut*> _shortcuts;
};

#endif /* DESKTOPWIN_H_ */
