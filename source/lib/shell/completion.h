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

#include <sys/common.h>
#include <shell/shell.h>
#include "exec/env.h"

#define TYPE_BUILTIN		0
#define TYPE_EXTERN			1
#define TYPE_PATH			2
#define TYPE_FUNCTION		3
#define TYPE_VARIABLE		4

#define APPS_DIR			"/bin/"

/* the builtin shell-commands */
typedef int (*fCommand)(int argc,char **argv);
typedef struct {
	uchar type;
	ushort mode;
	char path[MAX_PATH_LEN];
	char name[MAX_CMDNAME_LEN + 1];
	size_t nameLen;
	fCommand func;
	int complStart;
} sShellCmd;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Determines all matches for the given line
 *
 * @param e the environment
 * @param line the line
 * @param length the current cursorpos
 * @param max the maximum number of matches to collect
 * @param searchCmd whether you're looking for a command to execute
 * @return the matches or NULL if failed
 */
sShellCmd **compl_get(sEnv *e,char *str,size_t length,size_t max,bool searchCmd,bool searchPath);

/**
 * Free's the given matches
 *
 * @param matches the matches
 */
void compl_free(sShellCmd **matches);

#if defined(__cplusplus)
}
#endif
