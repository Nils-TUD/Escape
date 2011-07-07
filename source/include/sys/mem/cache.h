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

#ifndef CACHE_H_
#define CACHE_H_

#include <sys/common.h>

void *cache_alloc(size_t size);
void *cache_calloc(size_t num,size_t size);
void *cache_realloc(void *p,size_t size);
void cache_free(void *p);
size_t cache_getPageCount(void);
size_t cache_getOccMem(void);
size_t cache_getUsedMem(void);
void cache_print(void);

#endif /* CACHE_H_ */
