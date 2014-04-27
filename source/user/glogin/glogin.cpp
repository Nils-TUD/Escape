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
#include <gui/application.h>
#include <gui/graphics/color.h>
#include <gui/layout/flowlayout.h>
#include <gui/layout/gridlayout.h>
#include <gui/window.h>
#include <gui/combobox.h>
#include <gui/editable.h>
#include <gui/border.h>
#include <usergroup/user.h>
#include <usergroup/passwd.h>
#include <usergroup/group.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>

#define LOG_PATH		"/system/log"
#define DESKTOP_PROG	"desktop"

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
			  _users(user_parseFromFile(USERS_PATH,NULL)),
			  _pws(pw_parseFromFile(PASSWD_PATH,NULL)), _cbuser(), _edpw() {
		if(!_users)
			error("Unable to parse users from '%s'",USERS_PATH);
		if(!_pws)
			error("Unable to parse passwords from '%s'",PASSWD_PATH);

		getTheme().setPadding(0);

		shared_ptr<ColorFadePanel> header = make_control<ColorFadePanel>(make_layout<BorderLayout>(0));
		header->getTheme().setColor(Theme::CTRL_BACKGROUND,getTheme().getColor(Theme::WIN_TITLE_ACT_BG));
		header->add(make_control<Label>("Login",CENTER),BorderLayout::CENTER);
		add(header,BorderLayout::NORTH);

		shared_ptr<Panel> body = make_control<Panel>(
			make_layout<FlowLayout>(FRONT,true,VERTICAL,5));
		shared_ptr<Panel> namepnl = make_control<Panel>(make_layout<GridLayout>(2,1));
		namepnl->add(make_control<Label>("Username:"),GridPos(0,0));

		_cbuser = make_control<ComboBox>();
		_cbuser->changed().subscribe(mem_recv(this,&LoginPanel::onUserChanged));
		sUser *u = _users;
		while(u != NULL) {
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

	const sUser *getUser() const {
		return _user;
	}

private:
	static void indicateError(UIElement &el) {
		el.getTheme().setColor(Theme::CTRL_DARKBORDER,Color(0xFF,0,0));
		el.getTheme().setColor(Theme::CTRL_BORDER,Color(0xFF,0,0));
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
		sUser *u = user_getByName(_users,username->c_str());
		sPasswd *pw = pw_getById(_pws,u->uid);
		if(_edpw->getText() != pw->pw) {
			indicateError(*_edpw.get());
			return;
		}

		_user = u;
		Application::getInstance()->exit();
	}

	sUser *_user;
	sUser *_users;
	sPasswd *_pws;
	shared_ptr<ComboBox> _cbuser;
	shared_ptr<Editable> _edpw;
};

class DesktopWindow : public Window {
	static const Color BGCOLOR;

public:
	DesktopWindow(const Size &size) : Window(Pos(0,0),size,DESKTOP), _lgpnl() {
		shared_ptr<ColorFadePanel> bg = make_control<ColorFadePanel>();
		bg->getTheme().setColor(Theme::CTRL_BACKGROUND,BGCOLOR);

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

	const sUser *getUser() const {
		return _lgpnl->getUser();
	}

private:
	shared_ptr<LoginPanel> _lgpnl;
};

const Color DesktopWindow::BGCOLOR = Color(0xd5,0xe6,0xf3);

int main(void) {
	int fd;

	/* open stdin */
	if((fd = open(LOG_PATH,IO_READ)) != STDIN_FILENO)
		error("Unable to open '%s' for STDIN: Got fd %d",LOG_PATH,fd);

	/* open stdout */
	if((fd = open(LOG_PATH,IO_WRITE)) != STDOUT_FILENO)
		error("Unable to open '%s' for STDOUT: Got fd %d",LOG_PATH,fd);

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
	const sUser *u = win->getUser();
	Application::destroy();

	// set user- and group-id
	if(setgid(u->gid) < 0)
		error("Unable to set gid");
	if(setuid(u->uid) < 0)
		error("Unable to set uid");

	/* give this process hierarchy its own mountspace */
	if(clonems() < 0)
		error("Unable to clone mountspace");

	// cd to home-dir
	if(isdir(u->home))
		setenv("CWD",u->home);
	else
		setenv("CWD","/");
	setenv("HOME",getenv("CWD"));
	setenv("USER",u->name);

	/* read in groups */
	sGroup *groupList = group_parseFromFile(GROUPS_PATH,NULL);
	if(!groupList)
		error("Unable to parse groups from '%s'",GROUPS_PATH);
	/* determine groups and set them */
	size_t groupCount;
	gid_t *groups = group_collectGroupsFor(groupList,u->uid,1,&groupCount);
	if(!groups)
		error("Unable to collect group-ids");
	if(setgroups(groupCount,groups) < 0)
		error("Unable to set groups");

	// exec with desktop
	const char *args[] = {DESKTOP_PROG,NULL};
	execvp(DESKTOP_PROG,args);
	return EXIT_FAILURE;
}
