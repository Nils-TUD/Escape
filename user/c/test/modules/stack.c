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
#include <stdio.h>
#include <esc/proc.h>
#include <signal.h>
#include "stack.h"

static void sigHandler(tSig sig,u32 data);
static void f(int a);

int mod_stack(int argc,char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);
	if(setSigHandler(SIG_TERM,sigHandler) < 0)
		printe("Unable to set sig-handler");
	f(0);
	return 0;
}

static void sigHandler(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
}

static void f(int a) {
	if(a % 128 == 0)
		printf("&a = %08x\n",&a);
	if(sendSignalTo(getpid(),SIG_TERM,a) < 0)
		printe("Unable to send signal");
	f(a + 1);
}
