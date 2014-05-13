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

#include <esc/common.h>

template<typename T,typename Y>
T Atomic::fetch_and_add(T volatile *ptr,Y value) {
	return __sync_fetch_and_add(ptr,value);
}

template<typename T,typename Y>
T Atomic::fetch_and_or(T volatile *ptr,Y value) {
	return __sync_fetch_and_or(ptr,value);
}

template<typename T,typename Y>
T Atomic::fetch_and_and(T volatile *ptr,Y value) {
	return __sync_fetch_and_and(ptr,value);
}

template<typename T,typename Y>
bool Atomic::cmpnswap(T volatile *ptr,Y oldval,Y newval) {
	return __sync_bool_compare_and_swap(ptr,oldval,newval);
}
