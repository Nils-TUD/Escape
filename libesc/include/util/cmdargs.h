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

#ifndef CMDARGS_H_
#define CMDARGS_H_

#include <esc/common.h>
#include <util/iterator.h>
#include <stdarg.h>

typedef struct sCmdArgs sCmdArgs;
typedef void (*fCAParse)(sCmdArgs *a,const char *fmt,...);
typedef sIterator (*fCAFreeArgs)(sCmdArgs *a);
typedef void (*fCADestroy)(sCmdArgs *a);

struct sCmdArgs {
	int argc;
	char **argv;
	fCAParse parse;
	fCAFreeArgs getFreeArgs;
	fCADestroy destroy;
};

sCmdArgs *cmdargs_create(int argc,char **argv);
void cmdargs_destroy(sCmdArgs *a);

#endif /* CMDARGS_H_ */
