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

extern int mod_speed(int,char**);
extern int mod_driver(int,char**);
extern int mod_debug(int,char**);
extern int mod_fault(int,char**);
extern int mod_thread(int,char**);
extern int mod_fork(int,char**);
extern int mod_forkbomb(int,char**);
extern int mod_procswarm(int,char**);
extern int mod_mem(int,char**);
extern int mod_stack(int,char**);
extern int mod_tls(int,char**);
extern int mod_pipe(int,char**);
extern int mod_sigclone(int,char**);
extern int mod_fsreads(int,char**);
extern int mod_ooc(int,char**);
extern int mod_highcpu(int,char**);
extern int mod_drvparallel(int,char**);
extern int mod_maxthreads(int,char**);
extern int mod_pagefaults(int,char**);
