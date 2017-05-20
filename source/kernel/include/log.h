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

#include <vfs/file.h>
#include <vfs/vfs.h>
#include <common.h>
#include <ostream.h>
#include <spinlock.h>
#include <stdarg.h>

class Log : public OStream {
	static const size_t MAX_LOG_SIZE	= 1024 * 512;
	static const size_t BUF_SIZE		= 2048;

	class LogFile : public VFSFile {
	public:
		explicit LogFile(const fs::User &u,VFSNode *parent,bool &success)
				: VFSFile(u,parent,(char*)"log",FILE_DEF_MODE,success) {
		}
		virtual ssize_t write(pid_t pid,OpenFile *file,const void *buffer,off_t offset,size_t count);
	};

	explicit Log() : OStream(), buf(), bufPos(), logToSer(true), logFile(), vfsReady(), lock() {
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
	virtual uchar pipepad() const;
	virtual bool escape(const char **str);

	static void toSerial(char c);
	void flush();

	/* don't use a heap here to prevent problems */
	char buf[BUF_SIZE];
	size_t bufPos;
	bool logToSer;
	OpenFile *logFile;
	bool vfsReady;
	SpinLock lock;
	static Log inst;
};
