/**
 * $Id: streams.c 636 2010-05-01 15:09:28Z nasmussen $
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
#include <esc/io.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/exceptions/io.h>
#include <esc/io/ifilestream.h>
#include <esc/io/ofilestream.h>
#include <esc/io/iofilestream.h>
#include <stdio.h>

typedef void (*fConstr)(void);
typedef void (*fStreamClose)(void *);

static void streamConstr(void);
static void streamDestr(void);

fConstr constr[1] __attribute__((section(".ctors"))) = {
	streamConstr
};

sIStream *cin = NULL;
sOStream *cout = NULL;
sOStream *cerr = NULL;

FILE *stdin = NULL;
FILE *stdout = NULL;
FILE *stderr = NULL;

static void streamConstr(void) {
	cin = ifstream_openfd(STDIN_FILENO);
	cout = ofstream_openfd(STDOUT_FILENO);
	cerr = ofstream_openfd(STDERR_FILENO);
	stdin = (FILE*)iofstream_linkin(cin);
	stdout = (FILE*)iofstream_linkout(cout);
	stderr = (FILE*)iofstream_linkout(cerr);
	atexit(streamDestr);
}

static void streamDestr(void) {
	u32 i;
	fStreamClose funcs[3] = {
		(fStreamClose)cin->close,(fStreamClose)cout->close,(fStreamClose)cerr->close
	};
	void *streams[3] = {cin,cout,cerr};
	for(i = 0; i < 3; i++) {
		/* an uncatched io-exception is not good here since it may call exit() again */
		TRY {
			funcs[i](streams[i]);
		}
		CATCH(IOException,e) {
			debugf("Got an IO-Exception when destroying std-stream %d: %s\n",i,e->toString(e));
		}
		ENDCATCH
	}
}
