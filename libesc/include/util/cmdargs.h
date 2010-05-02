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

#ifndef ESCCMDARGS_H_
#define ESCCMDARGS_H_

#include <esc/common.h>
#include <util/iterator.h>
#include <sllist.h>
#include <stdarg.h>

#define CA_NO_DASHES	1		/* disallow '-' or '--' for arguments */
#define CA_REQ_EQ		2		/* require '=' for arguments */
#define CA_NO_EQ		4		/* disallow '=' for arguments */
#define CA_NO_FREE		8		/* throw exception if free arguments are found */
#define CA_MAX1_FREE	16		/* max. 1 free argument */

typedef struct sCmdArgs sCmdArgs;
typedef void (*fCAParse)(sCmdArgs *a,const char *fmt,...);
typedef sIterator (*fCAFreeArgs)(sCmdArgs *a);
typedef void (*fCADestroy)(sCmdArgs *a);
typedef const char *(*fCAGetFirstFree)(sCmdArgs *a);

struct sCmdArgs {
	int argc;
	const char **argv;
	u16 flags;
	u16 isHelp;
	sSLList *freeArgs;
	fCAParse parse;
	fCAGetFirstFree getFirstFree;
	fCAFreeArgs getFreeArgs;
	fCADestroy destroy;
};

sCmdArgs *cmdargs_create(int argc,const char **argv,u16 flags);
void cmdargs_destroy(sCmdArgs *a);

#endif /* ESCCMDARGS_H_ */
