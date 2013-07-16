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

#include <sys/common.h>
#include <sys/arch/i586/task/ioports.h>
#include <sys/task/proc.h>

int proc_cloneArch(sProc *dst,A_UNUSED const sProc *src) {
	ioports_init(dst);
	return 0;
}

void proc_terminateArch(sProc *p) {
	ioports_free(p);
}

size_t proc_getKMemUsageOf(sProc *p) {
	/* 1 pagedir, 1 page-table for kernel-stack, 1 kernelstack for each thread */
	return sll_length(&p->threads) + 2;
}
