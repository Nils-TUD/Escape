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

#include <common.h>
#include <arch/x86/gdt.h>
#include <arch/x86/task/ioports.h>
#include <task/smp.h>
#include <task/proc.h>
#include <mem/cache.h>
#include <video.h>
#include <errno.h>
#include <string.h>

void IOPorts::init(Proc *p) {
	p->ioMap = NULL;
}

int IOPorts::request(uint16_t start,size_t count) {
	Proc *p = Proc::request(Proc::getRunning(),PLOCK_PORTS);
	if(p->ioMap == NULL) {
		p->ioMap = (uint8_t*)Cache::alloc(TSS::IO_MAP_SIZE / 8);
		if(p->ioMap == NULL) {
			Proc::release(p,PLOCK_PORTS);
			return -ENOMEM;
		}
		/* mark all as disallowed */
		memset(p->ioMap,0xFF,TSS::IO_MAP_SIZE / 8);
	}

	/* 0 means allowed */
	while(count-- > 0) {
		p->ioMap[start / 8] &= ~(1 << (start % 8));
		start++;
	}

	GDT::setIOMap(p->ioMap,true);
	Proc::release(p,PLOCK_PORTS);
	return 0;
}

bool IOPorts::handleGPF() {
	bool res = false;
	Proc *p = Proc::request(Proc::getRunning(),PLOCK_PORTS);
	if(p->ioMap != NULL && !GDT::ioMapPresent()) {
		/* load it and give the process another try */
		GDT::setIOMap(p->ioMap,false);
		res = true;
	}
	Proc::release(p,PLOCK_PORTS);
	return res;
}

int IOPorts::release(uint16_t start,size_t count) {
	Proc *p = Proc::request(Proc::getRunning(),PLOCK_PORTS);
	if(p->ioMap == NULL) {
		Proc::release(p,PLOCK_PORTS);
		return -EINVAL;
	}

	/* 1 means disallowed */
	while(count-- > 0) {
		p->ioMap[start / 8] |= 1 << (start % 8);
		start++;
	}

	GDT::setIOMap(p->ioMap,true);
	Proc::release(p,PLOCK_PORTS);
	return 0;
}

void IOPorts::free(Proc *p) {
	if(p->ioMap != NULL) {
		Cache::free(p->ioMap);
		p->ioMap = NULL;
	}
}

/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void IOPorts::print(OStream &os,const uint8_t *map) {
	size_t c = 0;
	os.writef("Reserved IO-ports:\n\t");
	for(size_t i = 0; i < TSS::IO_MAP_SIZE / 8; i++) {
		for(size_t j = 0; j < 8; j++) {
			if(!(map[i] & (1 << j))) {
				os.writef("%zx, ",i * 8 + j);
				if(++c % 10 == 0)
					os.writef("\n\t");
			}
		}
	}
}

#endif
