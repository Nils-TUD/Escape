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

#ifndef SHELLAPP_H_
#define SHELLAPP_H_

#include <esc/common.h>
#include <esc/dir.h>
#include <esc/gui/common.h>
#include <esc/gui/application.h>
#include "shellcontrol.h"

using namespace esc::gui;

class ShellApplication : public Application {
public:
	ShellApplication(tServ sid,u32 no,ShellControl *sh) : Application(), _sid(sid), _sh(sh) {
		char *path = new char[MAX_PATH_LEN + 1];
		sprintf(path,"services:/guiterm%d",no);
		_selfFd = open(path,IO_WRITE);
		if(_selfFd < 0) {
			printe("Unable to open own service guiterm");
			exit(EXIT_FAILURE);
		}
		delete path;
		_inst = this;
	};
	virtual ~ShellApplication() {
	};

protected:
	void doEvents();

private:
	tServ _sid;
	tFD _selfFd;
	ShellControl *_sh;
};

#endif /* SHELLAPP_H_ */
