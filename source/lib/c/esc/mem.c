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

#include <esc/common.h>
#include <esc/mem.h>
#include <errno.h>

/* the assembler-routine */
extern ssize_t _changeSize(ssize_t count);
extern intptr_t _mapPhysical(uintptr_t *phys,size_t count,size_t align);
extern intptr_t _addRegion(sBinDesc *bin,uintptr_t binOffset,size_t byteCount,
		size_t loadCount,uint type);
extern intptr_t _createSharedMem(const char *name,size_t byteCount);
extern intptr_t _joinSharedMem(const char *name);

/* just a convenience for the user which sets errno if the return-value is zero (not enough mem) */
void *changeSize(ssize_t count) {
	size_t addr = _changeSize(count);
	if(addr == 0)
		errno = -ENOMEM;
	return (void*)addr;
}

void *mapPhysical(uintptr_t phys,size_t count) {
	intptr_t addr = _mapPhysical(&phys,count,1);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}

void *allocPhysical(uintptr_t *phys,size_t count,size_t align) {
	intptr_t addr = _mapPhysical(phys,count,align);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}

void *addRegion(sBinDesc *bin,uintptr_t binOffset,size_t byteCount,size_t loadCount,uint type) {
	intptr_t addr = _addRegion(bin,binOffset,byteCount,loadCount,type);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}

void *createSharedMem(const char *name,size_t byteCount) {
	intptr_t addr = _createSharedMem(name,byteCount);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}

void *joinSharedMem(const char *name) {
	intptr_t addr = _joinSharedMem(name);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}
