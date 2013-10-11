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
#include <sys/cppsupport.h>
#include <sys/semaphore.h>
#include <sys/ostream.h>

class Proc;

class Sems {
	Sems() = delete;

	static const size_t INIT_SEMS_COUNT		= 8;

public:
	struct Entry : public CacheAllocatable {
		explicit Entry(uint value) : refs(1), s(value) {
		}

		int refs;
		Semaphore s;
	};

	static int init(Proc *p);
	static int clone(Proc *p,const Proc *old);
	static int create(Proc *p,uint value);
	static int op(Proc *p,int sem,int amount);
	static void destroy(Proc *p,int sem);
	static void destroyAll(Proc *p,bool complete);
	static void print(OStream &os,const Proc *p);
};
