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
extern ssize_t _chgsize(ssize_t count);
extern intptr_t _mapphys(uintptr_t *phys,size_t count,size_t align);
extern intptr_t _mapmod(const char *name,size_t *size);
extern intptr_t _regadd(sBinDesc *bin,uintptr_t binOffset,size_t byteCount,
		size_t loadCount,uint type);
extern intptr_t _shmcrt(const char *name,size_t byteCount);
extern intptr_t _shmjoin(const char *name);

/* just a convenience for the user which sets errno if the return-value is zero (not enough mem) */
void *chgsize(ssize_t count) {
	size_t addr = _chgsize(count);
	if(addr == 0)
		errno = -ENOMEM;
	return (void*)addr;
}

void *mapphys(uintptr_t phys,size_t count) {
	intptr_t addr = _mapphys(&phys,count,1);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}

void *mapmod(const char *name,size_t *size) {
	intptr_t addr = _mapmod(name,size);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}

void *allocphys(uintptr_t *phys,size_t count,size_t align) {
	intptr_t addr = _mapphys(phys,count,align);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}

void *regadd(sBinDesc *bin,uintptr_t binOffset,size_t byteCount,size_t loadCount,uint type) {
	intptr_t addr = _regadd(bin,binOffset,byteCount,loadCount,type);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}

void *shmcrt(const char *name,size_t byteCount) {
	intptr_t addr = _shmcrt(name,byteCount);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}

void *shmjoin(const char *name) {
	intptr_t addr = _shmjoin(name);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}
