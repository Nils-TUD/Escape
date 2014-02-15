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
#include <sys/col/slist.h>
#include <sys/cppsupport.h>

class Proc;

class Env {
	Env() = delete;

public:
	struct EnvVar : public SListItem, public CacheAllocatable {
		explicit EnvVar(uint8_t dup,char *name,char *value)
			: SListItem(), dup(dup), name(name), value(value) {
		}
		uint8_t dup; /* whether this is a duplicate of a parent-var */
		char *name;
		char *value;
	};

	/**
	 * Copies the env-variable-name with given index to <dst>
	 *
	 * @param pid the process-id
	 * @param index the index
	 * @param dst the destination-string
	 * @param size the size of dst
	 * @return true if existing and successfully copied
	 */
	static bool geti(pid_t pid,size_t index,char *dst,size_t size);

	/**
	 * Copies the env-variable-value with given name to <dst>
	 *
	 * @param pid the process-id
	 * @param name the variable-name
	 * @param dst the destination-string
	 * @param size the size of dst
	 * @return true if existing and successfully copied
	 */
	static bool get(pid_t pid,const char *name,char *dst,size_t size);

	/**
	 * Sets <name> to <value>
	 *
	 * @param pid the process-id
	 * @param name the variable-name
	 * @param value the (new) value
	 * @return true if successfull
	 */
	static bool set(pid_t pid,const char *name,const char *value);

	/**
	 * Removes all env-variables for given process
	 *
	 * @param pid the process-id
	 */
	static void removeFor(pid_t pid);

	/**
	 * Prints all env-vars of given process
	 *
	 * @param os the output-stream
	 * @param pid the process-id
	 */
	static void printAllOf(OStream &os,pid_t pid);

private:
	static bool exists(pid_t pid,const char *name);
	static EnvVar *requestiVarOf(pid_t pid,size_t *index);
	static EnvVar *requestVarOf(pid_t pid,const char *name);
	static pid_t getPPid(pid_t pid);
	static SList<EnvVar> *request(pid_t pid);
	static void release();

	static SpinLock lock;
};
