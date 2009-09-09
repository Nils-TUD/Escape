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
#include <sllist.h>

typedef struct {
	char *source;
	bool writable;
} sAppDB;

typedef struct {
	u32 id;
	sAppDB *db;
	u16 appType;			/* driver, fs, service or default */
	sSLList *ioports;		/* list of sRange */
	sSLList *driver;		/* list of sDriverPerm */
	struct {
		bool read;
		bool write;
	} fs;
	sSLList *services;		/* list of names */
	sSLList *signals;		/* list of signal-numbers */
	sSLList *physMem;		/* list of sRange */
	sSLList *createShMem;	/* list of names */
	sSLList *joinShMem;		/* list of names */
} sApp;

void apps_init(void) {

}
