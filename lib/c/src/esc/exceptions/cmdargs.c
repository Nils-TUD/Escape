/**
 * $Id: cmdargs.c 641 2010-05-02 18:24:30Z nasmussen $
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
#include <esc/exceptions/cmdargs.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define MAX_EXMSG_LEN	64

static void ex_destroyCmdArgsException(sCmdArgsException *e);
static const char *ex_printCmdArgsException(sCmdArgsException *e);

sCmdArgsException *ex_createCmdArgsException(s32 id,s32 line,const char *file,const char *msg,...) {
	va_list ap;
	sCmdArgsException *e = (sCmdArgsException*)ex_create(id,line,file,sizeof(sCmdArgsException));
	e->_msg = malloc(MAX_EXMSG_LEN);
	if(!e->_msg)
		error("Unable to alloc memory for exception-msg (%s:%d)",file,line);
	va_start(ap,msg);
	vsnprintf(e->_msg,MAX_EXMSG_LEN,msg,ap);
	va_end(ap);
	e->destroy = (fExDestroy)ex_destroyCmdArgsException;
	e->toString = (fExToString)ex_printCmdArgsException;
	return e;
}

static void ex_destroyCmdArgsException(sCmdArgsException *e) {
	free(e->_msg);
	free(e);
}

static const char *ex_printCmdArgsException(sCmdArgsException *e) {
	UNUSED(e);
	return e->_msg;
}
