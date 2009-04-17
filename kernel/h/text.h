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

#ifndef TEXT_H_
#define TEXT_H_

#include "common.h"
#include <sllist.h>

/* usage-slot for a binary */
typedef struct {
	sSLList *procs;
	tInodeNo inodeNo;
	u32 modifytime;
} sTextUsage;

/**
 * Allocates the text for the current process
 *
 * @param path the path to the binary
 * @param file the file that is already open and can be used to load the text. Should NOT be closed!
 * @param position the position of the text in the file
 * @param textSize the size of the text in bytes
 * @param text will be set to the text-usage
 * @return 0 on success
 */
s32 text_alloc(const char *path,tFileNo file,u32 position,u32 textSize,sTextUsage **text);

/**
 * Releases the given text from the given process
 *
 * @param u the text-usage (not NULL)
 * @param pid the process-id
 * @return true if the text-frames should be free'd
 */
bool text_free(sTextUsage *u,tPid pid);

/**
 * Clones the given text for the given-process. That means the process will be added
 * to the text-usage.
 *
 * @param u the text-usage
 * @param pid the process-id
 */
void text_clone(sTextUsage *u,tPid pid);

#if DEBUGGING

/**
 * Prints the text-usages
 */
void text_dbg_print(void);

#endif

#endif /* TEXT_H_ */
