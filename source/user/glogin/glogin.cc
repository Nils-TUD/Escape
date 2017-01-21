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

#include <gui/graphics/color.h>
#include <gui/layout/flowlayout.h>
#include <gui/layout/gridlayout.h>
#include <gui/application.h>
#include <gui/border.h>
#include <gui/combobox.h>
#include <gui/editable.h>
#include <gui/window.h>
#include <sys/common.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <usergroup/usergroup.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>

using namespace gui;
using namespace std;

class ColorFadePanel : public Panel {
public:
	ColorFadePanel() : Panel() {
	}
	ColorFadePanel(shared_ptr<Layout> l) : Panel(l) {
	}

protected:
	virtual void paintBackground(Graphics &g) {
		const Color &bg = getTheme().getColor(Theme::CTRL_BACKGROUND);
		g.colorFadeRect(VERTICAL,bg,bg + 30,Pos(0,0),getSize());
	}
};

class LoginPanel : public Panel {
public:
	LoginPanel(Window &win)
			: Panel(make_layout<BorderLayout>(0)), _user(),
			  _users(usergroup_parse(USERS_PATH,NULL)), _cbuser(), _edpw() {
		if(!_users)
			error("Unable to parse users from '%s'",USERS_PATH);

		getTheme().setPadding(0);

		shared_ptr<ColorFadePanel> header = make_control<ColorFadePanel>(make_layout<BorderLayout>(0));
		header->getTheme().setColor(Theme::CTRL_BACKGROUND,Theme::WIN_TITLE_ACT_BG);
		header->add(make_control<Label>("Login",CENTER),BorderLayout::CENTER);
		add(header,BorderLayout::NORTH);

		shared_ptr<Panel> body = make_control<Panel>(
			make_layout<FlowLayout>(FRONT,true,VERTICAL,5));
		shared_ptr<Panel> namepnl = make_control<Panel>(make_layout<GridLayout>(2,1));
		namepnl->add(make_control<Label>("Username:"),GridPos(0,0));

		_cbuser = make_control<ComboBox>();
		_cbuser->changed().subscribe(mem_recv(this,&LoginPanel::onUserChanged));
		sNamedItem *u = _users;
		while(u != NULL) {
			// skip the users without PW
			if(usergroup_getPW(u->name))
				_cbuser->addItem(u->name);
			u = u->next;
		}
		namepnl->add(_cbuser,GridPos(1,0));
		win.appendTabCtrl(*_cbuser.get());
		body->add(namepnl);

		shared_ptr<Panel> pwpnl = make_control<Panel>(make_layout<GridLayout>(2,1));
		pwpnl->add(make_control<Label>("Password:"),GridPos(0,0));
		_edpw = make_control<Editable>(Pos(),Size(150,0),true);
		_edpw->keyPressed().subscribe(mem_recv(this,&LoginPanel::onKeyPress));
		pwpnl->add(_edpw,GridPos(1,0));
		win.appendTabCtrl(*_edpw.get());
		body->add(pwpnl);

		shared_ptr<Panel> btnpnl = make_control<Panel>(make_layout<FlowLayout>(BACK));
		shared_ptr<Button> login = make_control<Button>("Login");
		login->clicked().subscribe(mem_recv(this,&LoginPanel::onLogin));
		btnpnl->add(login);
		win.appendTabCtrl(*login.get());
		body->add(btnpnl);
		add(body,BorderLayout::CENTER);
	}

	const sNamedItem *getUser() const {
		return _user;
	}

private:
	static void indicateError(UIElement &el) {
		el.getTheme().setColor(Theme::CTRL_DARKBORDER,Theme::ERROR_COLOR);
		el.getTheme().setColor(Theme::CTRL_BORDER,Theme::ERROR_COLOR);
		el.repaint();
	}
	static void clearError(UIElement &el) {
		el.getTheme().unsetColor(Theme::CTRL_DARKBORDER);
		el.getTheme().unsetColor(Theme::CTRL_BORDER);
		el.repaint();
	}

	void onUserChanged(UIElement &) {
		clearError(*_cbuser);
	}
	void onKeyPress(UIElement &,const KeyEvent &e) {
		clearError(*_edpw);
		if(e.getKeyCode() == VK_ENTER)
			onLogin(*_edpw);
	}
	void onLogin(UIElement &) {
		// get user and check pw
		const string *username = _cbuser->getSelectedItem();
		if(!username) {
			indicateError(*_cbuser.get());
			return;
		}
		sNamedItem *u = usergroup_getByName(_users,username->c_str());
		const char *pw = usergroup_getPW(u->name);
		if(!pw || _edpw->getText() != pw) {
			indicateError(*_edpw.get());
			return;
		}

		_user = u;
		Application::getInstance()->exit();
	}

	sNamedItem *_user;
	sNamedItem *_users;
	shared_ptr<ComboBox> _cbuser;
	shared_ptr<Editable> _edpw;
};

class DesktopWindow : public Window {
public:
	DesktopWindow(const Size &size) : Window(Pos(0,0),size,DESKTOP), _lgpnl() {
		shared_ptr<ColorFadePanel> bg = make_control<ColorFadePanel>();
		bg->getTheme().setColor(Theme::CTRL_BACKGROUND,Theme::DESKTOP_BG);

		_lgpnl = make_control<LoginPanel>(*this);
		shared_ptr<Border> border = make_control<Border>(_lgpnl);
		bg->add(border);
		border->moveTo(Pos(size.width / 2 - border->getPreferredSize().width / 2,
			size.height / 2 - border->getPreferredSize().height / 2));
		border->resizeTo(border->getPreferredSize());

		getRootPanel()->setLayout(make_layout<BorderLayout>(0));
		getRootPanel()->getTheme().setPadding(0);
		getRootPanel()->add(bg,BorderLayout::CENTER);
	}

	const sNamedItem *getUser() const {
		return _lgpnl->getUser();
	}

private:
	shared_ptr<LoginPanel> _lgpnl;
};

int main(void) {
	const char *logfile = "/sys/log";

	int fd;

	/* open stdin */
	if((fd = open(logfile,O_RDONLY)) != STDIN_FILENO)
		error("Unable to open '%s' for STDIN: Got fd %d",logfile,fd);

	/* open stdout */
	if((fd = open(logfile,O_WRONLY)) != STDOUT_FILENO)
		error("Unable to open '%s' for STDOUT: Got fd %d",logfile,fd);

	/* dup stdout to stderr */
	if((fd = dup(fd)) != STDERR_FILENO)
		error("Unable to duplicate STDOUT to STDERR: Got fd %d",fd);

	// let the user login
	Application *app = Application::create();
	shared_ptr<DesktopWindow> win = make_control<DesktopWindow>(app->getScreenSize());
	win->show();
	app->addWindow(win);
	app->run();

	// get selected user and destroy app
	const sNamedItem *u = win->getUser();
	Application::destroy();

	/* set user and groups */
	if(usergroup_changeToName(u->name) < 0)
		error("Unable to change to user %s",u->name);

	// use a per-user mountspace
	char mspath[MAX_PATH_LEN];
	snprintf(mspath,sizeof(mspath),"/sys/ms/%s",u->name);
	int ms = open(mspath,O_RDONLY);
	if(ms < 0) {
		ms = open("/sys/proc/self/ms",O_RDONLY);
		if(ms < 0)
			error("Unable to open /sys/proc/self/ms for reading");
		if(clonems(ms,u->name) < 0)
			error("Unable to clone mountspace");
	}
	else {
		if(joinms(ms) < 0)
			error("Unable to join mountspace '%s'",mspath);
	}
	close(ms);

	// cd to home-dir
	char homedir[MAX_PATH_LEN];
	if(usergroup_getHome(u->name,homedir,sizeof(homedir)) != 0)
		error("Unable to get users home directory");
	if(isdir(homedir))
		setenv("CWD",homedir);
	else
		setenv("CWD","/");

	setenv("HOME",getenv("CWD"));
	setenv("USER",u->name);

	// exec with desktop
	const char *args[] = {"desktop",NULL};
	execvp(args[0],args);
	return EXIT_FAILURE;
}
