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
#include <sys/ostream.h>
#include <sys/vfs/vfs.h>
#include <stdarg.h>

class Log : public OStream {
	static const size_t BUF_SIZE		= 2048;

	explicit Log() : OStream(), buf(), bufPos(), col(), logToSer(true), logFile(), vfsReady(), lock() {
	}

public:
	/**
	 * @return the instance
	 */
	static Log &get() {
		return inst;
	}

	/**
	 * Tells the log that the VFS is usable now
	 */
	static void vfsIsReady();

	/**
	 * @return the file used for logging
	 */
	OpenFile *getFile();

	virtual void writec(char c);

private:
	virtual uchar pipepad();
	virtual bool escape(const char **str);

	static void toSerial(char c);
	static ssize_t write(pid_t pid,OpenFile *file,sVFSNode *n,const void *buffer,off_t offset,size_t count);
	void flush();

	/* don't use a heap here to prevent problems */
	char buf[BUF_SIZE];
	size_t bufPos;
	uint col;
	bool logToSer;
	OpenFile *logFile;
	bool vfsReady;
	klock_t lock;
	static Log inst;
};
