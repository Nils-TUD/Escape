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

#ifndef VIEW_H_
#define VIEW_H_

#include <sys/common.h>

/**
 * Lets the user view various kind of information about the current state of the system. This works
 * by calling *_dbg_print() or similar, capture the output and display it via cons_viewLines().
 *
 * @param argc the number of args
 * @param argv the arguments
 * @return 0 on success
 */
s32 cons_cmd_view(s32 argc,char **argv);

#endif /* VIEW_H_ */
