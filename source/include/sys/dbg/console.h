/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#pragma once

#include <sys/common.h>
#include <sys/dbg/lines.h>
#include <sys/video.h>

#define BYTES_PER_LINE		16
#define SCROLL_LINES		(VID_ROWS - 1)
#define SEARCH_NONE			0
#define SEARCH_FORWARD		1
#define SEARCH_BACKWARDS	2

/* to make a screen-backup */
typedef struct {
	char screen[VID_COLS * VID_ROWS * 2];
	ushort row;
	ushort col;
} sScreenBackup;

typedef uint8_t *(*fLoadLine)(void *data,uintptr_t addr);
typedef const char *(*fGetInfo)(void *data,uintptr_t addr);
typedef bool (*fLineMatches)(void *data,uintptr_t addr,const char *search,size_t searchlen);
typedef void (*fDisplayLine)(void *data,uintptr_t addr,uint8_t *bytes);
typedef uintptr_t (*fGotoAddr)(void *data,const char *gotoAddr);

typedef struct {
	fLoadLine loadLine;
	fGetInfo getInfo;
	fLineMatches lineMatches;
	fDisplayLine displayLine;
	fGotoAddr gotoAddr;
	uintptr_t startPos;
	uintptr_t maxPos;
} sNaviBackend;

/**
 * Starts the debugging-console
 */
void cons_start(void);

/**
 * Enables/disables writing to log
 *
 * @param enabled the new value
 */
void cons_setLogEnabled(bool enabled);

/**
 * Prints a dump of one line from <bytes> whose origin is <addr>.
 *
 * @param addr the address
 * @param bytes the bytes to print. has to contain at least BYTES_PER_LINE bytes!
 */
void cons_dumpLine(uintptr_t addr,uint8_t *bytes);

/**
 * Runs a navigation loop, i.e. reads from keyboard and lets the user navigate via certain key-
 * strokes. It calls backend->loadLine() to get the content.
 *
 * @param backend the backend to use
 * @param data will be passed to all backend-functions
 */
void cons_navigation(const sNaviBackend *backend,void *data);

/**
 * Displays the given lines and provides a navigation through them
 *
 * @param l the lines
 */
void cons_viewLines(const sLines *l);

/**
 * Uses <backend> to search for matches that may span multiple lines.
 *
 * @param backend the backend
 * @param data will be passed to all backend-functions
 * @param addr the current address
 * @param search the term to search for
 * @param searchlen the length of the search term
 * @return true if the line @ <addr> matches
 */
bool cons_multiLineMatches(const sNaviBackend *backend,void *data,uintptr_t addr,
                           const char *search,size_t searchlen);
