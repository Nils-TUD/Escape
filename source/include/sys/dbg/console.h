/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <sys/videolog.h>
#include <sys/cppsupport.h>
#include <string.h>

#define CONS_EXIT			-1234
/* has to be a power of 2 */
#if defined(__mmix__) || defined(__x86_64__)
#	define BYTES_PER_LINE		8
#else
#	define BYTES_PER_LINE		16
#endif
#define SCROLL_LINES		(VID_ROWS - 1)
#define SEARCH_NONE			0
#define SEARCH_FORWARD		1
#define SEARCH_BACKWARDS	2

/* to make a screen-backup */
struct ScreenBackup {
	char screen[VID_COLS * VID_ROWS * 2];
	ushort row;
	ushort col;
};

class NaviBackend : public CacheAllocatable {
public:
	explicit NaviBackend(uintptr_t startPos,uintptr_t maxPos)
		: startPos(startPos), maxPos(maxPos) {
	}
	virtual ~NaviBackend() {
	}

	uintptr_t getStartPos() const {
		return startPos;
	}
	uintptr_t getMaxPos() const {
		return maxPos;
	}

	virtual uint8_t *loadLine(uintptr_t addr) = 0;
	virtual const char *getInfo(uintptr_t addr) = 0;
	virtual bool lineMatches(uintptr_t addr,const char *search,size_t searchlen) = 0;
	virtual void displayLine(OStream &os,uintptr_t addr,uint8_t *bytes) = 0;
	virtual uintptr_t gotoAddr(const char *gotoAddr) = 0;

private:
	uintptr_t startPos;
	uintptr_t maxPos;
};

class Console {
	Console() = delete;

	static const size_t HISTORY_SIZE	= 32;
	static const size_t MAX_ARG_COUNT	= 5;
	static const size_t MAX_ARG_LEN		= 32;
	static const size_t MAX_SEARCH_LEN	= 16;

public:
	typedef int (*command_func)(OStream &os,size_t argc,char **argv);

	struct Command {
		char name[MAX_ARG_LEN];
		command_func exec;
	};

	/**
	 * Starts the debugging-console
	 *
	 * @param initialcmd the initial command to execute (NULL = none)
	 */
	static void start(const char *initialcmd);

	/**
	 * @param argc the number of args
	 * @param argv the args
	 * @return true if the given arguments contain a help-request
	 */
	static bool isHelp(int argc,char **argv) {
		return argc > 1 && (strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"-?") == 0 ||
				strcmp(argv[1],"--help") == 0);
	}

	/**
	 * Enables/disables writing to log
	 *
	 * @param enabled the new value
	 */
	static void setLogEnabled(bool enabled) {
		if(enabled)
			out = &vidlog;
		else
			out = &Video::get();
	}

	/**
	 * Prints a dump of one line from <bytes> whose origin is <addr>.
	 *
	 * @param os the output-stream
	 * @param addr the address
	 * @param bytes the bytes to print. has to contain at least BYTES_PER_LINE bytes!
	 */
	static void dumpLine(OStream &os,uintptr_t addr,uint8_t *bytes);

	/**
	 * Runs a navigation loop, i.e. reads from keyboard and lets the user navigate via certain key-
	 * strokes. It calls backend->loadLine() to get the content.
	 *
	 * @param os the output-stream
	 * @param backend the backend to use
	 */
	static void navigation(OStream &os,NaviBackend *backend);

	/**
	 * Displays the given lines and provides a navigation through them
	 *
	 * @param os the output-stream
	 * @param l the lines
	 */
	static void viewLines(OStream &os,const Lines *l);

	/**
	 * Uses <backend> to search for matches that may span multiple lines.
	 *
	 * @param backend the backend
	 * @param addr the current address
	 * @param search the term to search for
	 * @param searchlen the length of the search term
	 * @return true if the line @ <addr> matches
	 */
	static bool multiLineMatches(NaviBackend *backend,uintptr_t addr,const char *search,size_t searchlen);

private:
	static int help(OStream &os,size_t argc,char **argv);
	static int exit(OStream &os,size_t argc,char **argv);
	static int cont(OStream &os,size_t argc,char **argv);
	static uintptr_t getMaxAddr(uintptr_t end);
	static uintptr_t incrAddr(uintptr_t end,uintptr_t addr,size_t amount);
	static uintptr_t decrAddr(uintptr_t addr,size_t amount);
	static void display(OStream &os,NaviBackend *backend,const char *searchInfo,const char *search,
	                    int searchMode,uintptr_t *addr);
	static uint8_t charToInt(char c);
	static void convSearch(const char *src,char *dst,size_t len);
	static char **parseLine(const char *line,size_t *argc);
	static char *readLine();
	static Command *getCommand(const char *name);

	static size_t histWritePos;
	static size_t histReadPos;
	static size_t histSize;
	static char *history[HISTORY_SIZE];
	static char emptyLine[VID_COLS];
	static ScreenBackup backup;
	static ScreenBackup viewBackup;
	static OStream *out;
	static VideoLog vidlog;
	static Command commands[];
};
