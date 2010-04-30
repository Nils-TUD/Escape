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
#include <esc/setjmp.h>
#include <exceptions/exception.h>

#define MAX_TRIES					16

sException *__exPtr = NULL;
static int tryNo = 0;
static sJumpEnv tries[MAX_TRIES];

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
