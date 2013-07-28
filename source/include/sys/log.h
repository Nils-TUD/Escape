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
#include <sys/spinlock.h>
#include <sys/printf.h>
#include <sys/vfs/vfs.h>
#include <stdarg.h>

class Log {
	Log() = delete;

	static const size_t BUF_SIZE		= 2048;

public:
	/**
	 * @return the file used for logging
	 */
	static sFile *getFile();

	/**
	 * Tells the log that the VFS is usable now
	 */
	static void vfsIsReady();

	/**
	 * Formatted output into the logfile
	 *
	 * @param fmt the format
	 */
	static void printf(const char *fmt,...) {
		va_list ap;
		va_start(ap,fmt);
		vprintf(fmt,ap);
		va_end(ap);
	}

	/**
	 * Like printf, but with argument-list
	 *
	 * @param fmt the format
	 * @param ap the argument-list
	 */
	static void vprintf(const char *fmt,va_list ap) {
		/* lock it all, to make the debug-output more readable */
		SpinLock::acquire(&lock);
		prf_vprintf(&env,fmt,ap);
		flush();
		SpinLock::release(&lock);
	}

private:
	/**
	 * Writes the given character to log
	 *
	 * @param c the char
	 */
	static void writeChar(char c);

	static void printc(char c);
	static uchar pipePad();
	static void escape(const char **str);
	static ssize_t write(pid_t pid,sFile *file,sVFSNode *n,const void *buffer,
			off_t offset,size_t count);
	static void flush();

	/* don't use a heap here to prevent problems */
	static char buf[BUF_SIZE];
	static size_t bufPos;
	static uint col;
	static bool logToSer;
	static sFile *logFile;
	static bool vfsReady;
	static klock_t lock;
	static sPrintEnv env;
};
