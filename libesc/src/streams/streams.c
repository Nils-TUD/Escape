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
#include <esc/io.h>
#include <esc/proc.h>
#include <streams/ifilestream.h>
#include <streams/ofilestream.h>

typedef void (*fConstr)(void);

static void streamConstr(void);
static void streamDestr(void);

fConstr constr[1] __attribute__((section(".ctors"))) = {
	streamConstr
};

sIStream *cin = NULL;
sOStream *cout = NULL;
sOStream *cerr = NULL;

static void streamConstr(void) {
	cin = ifstream_openfd(STDIN_FILENO);
	cout = ofstream_openfd(STDOUT_FILENO);
	cerr = ofstream_openfd(STDERR_FILENO);
	atexit(streamDestr);
}

static void streamDestr(void) {
	cin->close(cin);
	cout->close(cout);
	cerr->close(cerr);
}
