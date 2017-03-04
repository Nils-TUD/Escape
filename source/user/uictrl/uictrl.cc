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

#include <esc/proto/initui.h>
#include <esc/proto/richui.h>
#include <esc/stream/std.h>
#include <esc/env.h>
#include <info/ui.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <dirent.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <map>

using namespace esc;

static const struct {
	esc::InitUI::Type type;
	const char *name;
} types[] = {
	{esc::InitUI::TUI, "TUI"},
	{esc::InitUI::GUI, "GUI"},
};
static const char *typenames[] = {
	"", "TUI", "GUI"
};

static const char *KEYMAP_DIR = "/etc/keymaps";
static std::map<info::ui::id_type,info::ui> uis;

static void usage(const char *name);

static esc::Screen::Mode modeById(const std::vector<esc::Screen::Mode> &modes,int id) {
	for(auto &m : modes) {
		if(m.id == id)
			return m;
	}
	error("Unable to find mode %d",id);
}

static void cmd_list() {
	std::vector<esc::Screen::Mode> modes = esc::RichUI(esc::env::get("UI").c_str()).getModes();

	sout << "id  type keymap mode\n";
	for(auto &ui : uis) {
		char path[MAX_PATH_LEN];
		strnzcpy(path,ui.second.keymap().c_str(),sizeof(path));
		const char *km = basename(path);

		esc::Screen::Mode mode = modeById(modes,ui.second.mode());
		sout << fmt(ui.first,"-",3) << " "
			 << typenames[ui.second.type()] << "  "
			 << fmt(km,"-",6) << " "
			 << mode.id
			 << ": " << fmt(mode.width,4)
			 << "x" << fmt(mode.height,4)
			 << "x" << mode.bitsPerPixel << "\n";
	}
}

static void cmd_start(int argc,char **argv) {
	if(argc != 3)
		usage(argv[0]);

	esc::InitUI::Type type = esc::InitUI::TUI;
	for(size_t i = 0; i < ARRAY_SIZE(types); ++i) {
		if(strcmp(argv[2],types[i].name) == 0) {
			type = types[i].type;
			break;
		}
	}

	esc::InitUI init("/dev/initui");
	for(info::ui::id_type id = 0; id < esc::UI::MAX_UIS; ++id) {
		if(uis.find(id) == uis.end()) {
			char path[MAX_PATH_LEN];
			snprintf(path,sizeof(path),"ui%zu",id);
			init.start(type,path);
			return;
		}
	}

	error("All UIs are in use");
}

static void cmd_modes() {
	esc::RichUI ui(esc::env::get("UI").c_str());
	std::vector<esc::Screen::Mode> modes = ui.getModes();
	esc::Screen::Mode curMode = ui.getMode();

	sout << "Available modes:\n";
	for(auto it = modes.begin(); it != modes.end(); ++it) {
		sout << (curMode.id == it->id ? '*' : ' ') << " ";
		sout << fmt(it->id,5) << ": ";
		sout << fmt(it->cols,3) << " x " << fmt(it->rows,3) << " cells, ";
		sout << fmt(it->width,4) << " x " << fmt(it->height,4) << " pixels, ";
		sout << fmt(it->bitsPerPixel,2) << "bpp, ";
		sout << (it->mode == esc::Screen::MODE_TEXT ? "text     " : "graphical") << " ";
		sout << "(" << ((it->type & esc::Screen::MODE_TYPE_TUI) ? "tui" : "-") << ",";
		sout << ((it->type & esc::Screen::MODE_TYPE_GUI) ? "gui" : "-") << ")";
		sout << "\n";
	}
}

static void cmd_setmode(int argc,char **argv) {
	if(argc != 3)
		usage(argv[0]);

	esc::RichUI ui(esc::env::get("UI").c_str());
	ui.requestMode(atoi(argv[2]));
}

static void cmd_keymaps() {
	esc::RichUI ui(esc::env::get("UI").c_str());

	struct stat curInfo;
	std::string keymap = ui.getKeymap();
	if(stat(keymap.c_str(),&curInfo) < 0)
		errmsg("Unable to stat current keymap (" << keymap << ")");

	struct dirent e;
	DIR *dir = opendir(KEYMAP_DIR);
	if(!dir)
		exitmsg("Unable to open '" << KEYMAP_DIR << "'");
	while(readdirto(dir,&e)) {
		if(strcmp(e.d_name,".") != 0 && strcmp(e.d_name,"..") != 0) {
			char fpath[MAX_PATH_LEN];
			snprintf(fpath,sizeof(fpath),"%s/%s",KEYMAP_DIR,e.d_name);
			struct stat finfo;
			if(stat(fpath,&finfo) < 0)
				errmsg("Unable to stat '" << fpath << "'");

			if(finfo.st_ino == curInfo.st_ino && finfo.st_dev == curInfo.st_dev)
				sout << "* " << e.d_name << "\n";
			else
				sout << "  " << e.d_name << "\n";
		}
	}
	closedir(dir);
}

static void cmd_setkeymap(int argc,char **argv) {
	if(argc < 3)
		usage(argv[0]);

	esc::RichUI ui(esc::env::get("UI").c_str());
	ui.setKeymap(std::string(KEYMAP_DIR) + "/" + argv[2]);
}

static void usage(const char *name) {
	serr << "Usage: " << name << " <cmd> ...\n";
	serr << "    list            : list all UIs\n";
	serr << "    start <type>    : start new UI; type is TUI or GUI\n";
	serr << "\n";
	serr << "Operations with the current UI:\n";
	serr << "    modes           : list all screen modes\n";
	serr << "    setmode <mode>  : set screen mode\n";
	serr << "    keymaps         : list all keymaps\n";
	serr << "    setkeymap <name>: set keymap\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	if(getopt_ishelp(argc,argv))
		usage(argv[0]);

	uis = info::ui::get_list();

	if(argc < 2 || strcmp(argv[1],"list") == 0)
		cmd_list();
	else if(strcmp(argv[1],"start") == 0)
		cmd_start(argc,argv);
	else if(strcmp(argv[1],"modes") == 0)
		cmd_modes();
	else if(strcmp(argv[1],"setmode") == 0)
		cmd_setmode(argc,argv);
	else if(strcmp(argv[1],"keymaps") == 0)
		cmd_keymaps();
	else if(strcmp(argv[1],"setkeymap") == 0)
		cmd_setkeymap(argc,argv);
	else
		usage(argv[0]);
	return EXIT_SUCCESS;
}
