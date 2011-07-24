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
#include <sys/arch/i586/gdt.h>
#include <sys/arch/i586/task/vm86.h>
#include <sys/arch/i586/task/ioports.h>
#include <sys/arch/i586/syscalls/proc.h>
#include <sys/mem/paging.h>
#include <sys/task/proc.h>
#include <sys/syscalls.h>
#include <errors.h>

int sysc_requestIOPorts(sIntrptStackFrame *stack) {
	uint16_t start = SYSC_ARG1(stack);
	size_t count = SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	int err;

	/* check range */
	if(count == 0 || count > 0xFFFF || (uint32_t)start + count > 0xFFFF)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	err = ioports_request(p,start,count);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,0);
}

int sysc_releaseIOPorts(sIntrptStackFrame *stack) {
	uint16_t start = SYSC_ARG1(stack);
	size_t count = SYSC_ARG2(stack);
	sProc *p;
	int err;

	/* check range */
	if(count == 0 || count > 0xFFFF || (uint32_t)start + count > 0xFFFF)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	p = proc_getRunning();
	err = ioports_release(p,start,count);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,0);
}

int sysc_vm86int(sIntrptStackFrame *stack) {
	uint16_t interrupt = (uint16_t)SYSC_ARG1(stack);
	sVM86Regs *regs = (sVM86Regs*)SYSC_ARG2(stack);
	sVM86Memarea *mAreas = (sVM86Memarea*)SYSC_ARG3(stack);
	size_t mAreaCount = (size_t)SYSC_ARG4(stack);
	int res;

	/* check args */
	if(regs == NULL)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isRangeUserWritable((uint32_t)regs,sizeof(sVM86Regs)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(mAreas != NULL) {
		size_t i;
		if(!paging_isRangeUserReadable((uint32_t)mAreas,sizeof(sVM86Memarea) * mAreaCount))
			SYSC_ERROR(stack,ERR_INVALID_ARGS);
		for(i = 0; i < mAreaCount; i++) {
			/* ensure that only memory from the real-mode-memory can be copied */
			if(mAreas[i].type == VM86_MEM_DIRECT) {
				if(mAreas[i].data.direct.dst + mAreas[i].data.direct.size < mAreas[i].data.direct.dst ||
					mAreas[i].data.direct.dst + mAreas[i].data.direct.size >= (1 * M + 64 * K))
					SYSC_ERROR(stack,ERR_INVALID_ARGS);
				if(!paging_isRangeUserWritable((uint32_t)mAreas[i].data.direct.src,
						mAreas[i].data.direct.size)) {
					SYSC_ERROR(stack,ERR_INVALID_ARGS);
				}
			}
			else {
				if(!paging_isRangeUserReadable((uint32_t)mAreas[i].data.ptr.srcPtr,sizeof(void*)))
					SYSC_ERROR(stack,ERR_INVALID_ARGS);
				if(!paging_isRangeUserWritable(mAreas[i].data.ptr.result,mAreas[i].data.ptr.size))
					SYSC_ERROR(stack,ERR_INVALID_ARGS);
			}
		}
	}
	else
		mAreaCount = 0;

	/* do vm86-interrupt */
	res = vm86_int(interrupt,regs,mAreas,mAreaCount);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
