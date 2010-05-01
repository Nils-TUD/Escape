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

#include <esc/common.h>
#include <esc/heap.h>
#include <exceptions/cmdargs.h>

static const char *ex_printCmdArgsException(sCmdArgsException *e);

sCmdArgsException *ex_createCmdArgsException(s32 id,s32 line,const char *file) {
	sCmdArgsException *e = (sCmdArgsException*)malloc(sizeof(sCmdArgsException));
	if(!e)
		error("Unable to create exception-object (%s:%d)",file,line);
	e->handled = 0;
	e->id = id;
	e->line = line;
	e->file = file;
	e->toString = (fToString)ex_printCmdArgsException;
	return e;
}

static const char *ex_printCmdArgsException(sCmdArgsException *e) {
	UNUSED(e);
	return "An required argument is missing";
}
