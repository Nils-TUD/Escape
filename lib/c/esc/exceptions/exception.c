/**
 * $Id: exception.c 641 2010-05-02 18:24:30Z nasmussen $
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
#include <esc/setjmp.h>
#include <esc/exceptions/exception.h>

#define MAX_TRIES					16

static void ex_destroy(sException *e);
static const char *ex_toString(sException *e);

sException *__curEx = NULL;
sException *__exPtr = NULL;
static int tryNo = 0;
static sJumpEnv tries[MAX_TRIES];

sException *ex_create(s32 id,s32 line,const char *file,u32 size) {
	sException *e = (sException*)malloc(size);
	if(!e)
		error("Unable to create exception-object (%s:%d)",file,line);
	e->_handled = 0;
	e->_id = id;
	*(s32*)&e->line = line;
	*(const char**)&e->file = file;
	e->destroy = (fExDestroy)ex_destroy;
	e->toString = (fExToString)ex_toString;
	return e;
}

sJumpEnv *ex_push(void) {
	return tries + tryNo++;
}

void ex_pop(void) {
	if(tryNo > 0)
		tryNo--;
}

void ex_unwind(void *exObj) {
	if(tryNo > 0)
		longjmp(tries + --tryNo,1);
	else {
		sException *e = (sException*)exObj;
		error("Unhandled exception in %s line %d: %s",e->file,e->line,e->toString(e));
	}
}

static void ex_destroy(sException *e) {
	free(e);
}

static const char *ex_toString(sException *e) {
	UNUSED(e);
	return "";
}
