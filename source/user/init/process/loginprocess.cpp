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

#include <esc/common.h>
#include <esc/proc.h>
#include <stdlib.h>
#include <stdio.h>

#include "loginprocess.h"
#include "../initerror.h"

void LoginProcess::load() {
	_pid = fork();
	if(_pid == 0) {
		const char *argv[] = {"/bin/login",nullptr,nullptr};
		argv[1] = _vterm.c_str();

		/* close stdin,stdout and stderr */
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

		exec(argv[0],argv);
		error("Exec of '%s' failed",argv[0]);
	}
	else if(_pid < 0)
		throw init_error("Fork failed");
}

