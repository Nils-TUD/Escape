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

#include <gui/graphics/color.h>
#include <gui/layout/borderlayout.h>
#include <gui/layout/flowlayout.h>
#include <gui/image.h>
#include <gui/imagebutton.h>
#include <gui/window.h>
#include <sys/common.h>
#include <map>

class Shortcut {
	friend class DesktopWin;

public:
	Shortcut(const std::string& icon,const std::string& app)
		: _icon(icon), _app(app), _btn() {
	}

	const std::string& getIcon() const {
		return _icon;
	}
	const std::string& getApp() const {
		return _app;
	}

private:
	std::shared_ptr<gui::ImageButton> getButton() const {
		return _btn;
	}
	void setButton(std::shared_ptr<gui::ImageButton> btn) {
		_btn = btn;
	}

private:
	std::string _icon;
	std::string _app;
	std::shared_ptr<gui::ImageButton> _btn;
};

class DesktopWin : public gui::Window {
	class WinButton : public gui::Button {
	public:
		WinButton(DesktopWin *inst,const std::string &title)
			: Button(title), _sub(clicked(),mem_recv(inst,&DesktopWin::onWinBtnClicked)) {
		}

	private:
		onclick_type::subscr_type _sub;
	};

	class Background : public gui::Panel {
	public:
		Background(std::shared_ptr<gui::Layout> l) : gui::Panel(l) {
		}

	protected:
		virtual void paintBackground(gui::Graphics &g) {
			const gui::Color &bg = getTheme().getColor(gui::Theme::CTRL_BACKGROUND);
			g.colorFadeRect(gui::VERTICAL,bg,bg + 30,gui::Pos(0,0),getSize());
		}
	};

public:
	static const gsize_t PADDING;
	static const gsize_t ICON_SIZE;
	static const gui::Color BGCOLOR;
	static const gui::Color ACTIVE_COLOR;
	static const gsize_t TASKBAR_HEIGHT;

public:
	DesktopWin(const gui::Size &size,int childsm);

	void addShortcut(Shortcut *sc) {
		// do that first for exception-safety
		std::shared_ptr<gui::Image> img = gui::Image::loadImage(sc->getIcon());
		std::shared_ptr<gui::ImageButton> btn(gui::make_control<gui::ImageButton>(img));
		sc->setButton(btn);
		btn->clicked().subscribe(mem_recv(this,&DesktopWin::onIconClick));
		_shortcuts[btn] = sc;
		_iconPanel->add(btn);
		_iconPanel->repaint();
	}

	// no cloning
private:
	DesktopWin(const DesktopWin &w);
	DesktopWin &operator=(const DesktopWin &w);

	void onWinBtnClicked(UIElement& el);
	void onWindowCreated(gwinid_t id,const std::string& title);
	void onWindowActive(gwinid_t id);
	void onWindowDestroyed(gwinid_t id);

	void init();
	void onIconClick(UIElement& el);

private:
	int _childsm;
	std::shared_ptr<gui::Panel> _winPanel;
	std::shared_ptr<Background> _iconPanel;
	WinButton *_active;
	std::map<gwinid_t,std::shared_ptr<WinButton>> _windows;
	std::map<std::shared_ptr<gui::ImageButton>,Shortcut*> _shortcuts;
};
