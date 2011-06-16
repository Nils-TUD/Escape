/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <sys/common.h>
#include <sys/mem/dynarray.h>

void dyna_start(sDynArray *d,size_t objSize,uintptr_t areaBegin,size_t areaSize) {
	d->objSize = objSize;
	d->objCount = 0;
	d->_areaBegin = areaBegin;
	d->_areaSize = areaSize;
	d->_regions = NULL;
}

void *dyna_getObj(sDynArray *d,size_t index) {
	sDynaRegion *reg = d->_regions;
	/* note that we're using the index here to prevent that an object reaches out of a region */
	while(reg != NULL) {
		size_t objsInReg = reg->size / d->objSize;
		if(index < objsInReg)
			return (void*)(reg->addr + index * d->objSize);
		index -= objsInReg;
		reg = reg->next;
	}
	return NULL;
}

ssize_t dyna_getIndex(sDynArray *d,const void *obj) {
	sDynaRegion *reg = d->_regions;
	size_t index = 0;
	while(reg != NULL) {
		if((uintptr_t)obj >= reg->addr && (uintptr_t)obj < reg->addr + reg->size)
			return index + ((uintptr_t)obj - reg->addr) / d->objSize;
		index += reg->size / d->objSize;
		reg = reg->next;
	}
	return -1;
}
