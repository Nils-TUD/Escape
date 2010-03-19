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

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <esc/common.h>
#include "shell.h"

#define TYPE_BUILTIN		0
#define TYPE_EXTERN			1
#define TYPE_PATH			2

#define APPS_DIR			"/bin/"

/* the builtin shell-commands */
typedef s32 (*fCommand)(u32 argc,char **argv);
typedef struct {
	u8 type;
	u16 mode;
	char name[MAX_CMDNAME_LEN + 1];
	fCommand func;
	s32 complStart;
} sShellCmd;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Determines all matches for the given line
 *
 * @param line the line
 * @param length the current cursorpos
 * @param max the maximum number of matches to collect
 * @param searchCmd whether you're looking for a command to execute
 * @return the matches or NULL if failed
 */
sShellCmd **compl_get(char *str,u32 length,u32 max,bool searchCmd,bool searchPath);

/**
 * Free's the given matches
 *
 * @param matches the matches
 */
void compl_free(sShellCmd **matches);

#ifdef __cplusplus
}
#endif

#endif /* COMMANDS_H_ */
