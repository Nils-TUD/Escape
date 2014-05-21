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

EXTERN_C void *cache_alloc(size_t size);
EXTERN_C void *cache_calloc(size_t num,size_t size);
EXTERN_C void *cache_realloc(void *p,size_t size);
EXTERN_C void cache_free(void *p);

EXTERN_C void *slln_allocNode(size_t size);
EXTERN_C void slln_freeNode(void *o);

EXTERN_C void util_panic(const char *fmt,...);

EXTERN_C void vid_printf(const char *fmt,...);
EXTERN_C void vid_vprintf(const char *fmt,va_list ap);
