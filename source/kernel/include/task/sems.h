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

#include <common.h>
#include <cppsupport.h>
#include <semaphore.h>
#include <ostream.h>

class Proc;

class Sems {
	Sems() = delete;

	static const size_t INIT_SEMS_COUNT		= 8;

public:
	struct Entry : public CacheAllocatable {
		explicit Entry(uint value,int irq) : refs(1), irq(irq), s(value) {
		}

		int refs;
		int irq;
		Semaphore s;
	};

	static int init(Proc *p);
	static int clone(Proc *p,const Proc *old);
	static int create(Proc *p,uint value,int irq = -1,const char *name = NULL,
		uint64_t *msiaddr = NULL,uint32_t *msival = NULL);
	static int op(Proc *p,int sem,int amount);
	static void destroy(Proc *p,int sem);
	static void destroyAll(Proc *p,bool complete);
	static void print(OStream &os,const Proc *p);

private:
	static void unref(Entry *e);
};
