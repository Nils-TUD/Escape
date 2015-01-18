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

#include <sys/common.h>

extern int mod_getpid(int,char**);
extern int mod_yield(int,char**);
extern int mod_fork(int,char**);
extern int mod_startthread(int,char**);
extern int mod_file(int,char**);
extern int mod_mmap(int,char**);
extern int mod_sendrecv(int,char**);
extern int mod_pingpong(int,char**);
extern int mod_pipe(int,char**);
extern int mod_reading(int,char**);
extern int mod_writenull(int,char**);
extern int mod_memops(int,char**);
extern int mod_locks(int,char**);
extern int mod_chgsize(int,char**);
extern int mod_pagefault(int,char**);
extern int mod_heap(int,char**);
extern int mod_stdio(int,char**);
