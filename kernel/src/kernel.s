#
# $Id$
# Copyright (C) 2008 - 2009 Nils Asmussen
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

# exports
.global gdt_flush
.global tss_load
.global util_outByte
.global util_outWord
.global util_outDWord
.global util_inByte
.global util_inWord
.global util_inDWord
.global util_halt
.global intrpt_setEnabled
.global intrpt_loadidt
.global paging_enable
.global paging_flushTLB
.global paging_exchangePDir
.global cpu_cpuidSupported
.global cpu_rdtsc
.global cpu_getInfo
.global cpu_getStrInfo
.global cpu_getCR0
.global cpu_setCR0
.global cpu_getCR2
.global cpu_getCR3
.global cpu_getCR4
.global cpu_setCR4
.global fpu_finit
.global fpu_saveState
.global fpu_restoreState
.global thread_save
.global thread_resume
.global getStackFrameStart
.global kernelStack
.global higherhalf

# imports
.extern main
.extern intrpt_handler
.extern entryPoint

# general constants
# TODO better way which uses the defines from paging.h?
.set PAGE_SIZE,								4096
.set KERNEL_STACK,						0xFF7FF000
.set KERNEL_STACK_PTE,				0xFFFFDFFC
.set TMP_STACK_SIZE,					PAGE_SIZE
.set USER_STACK,							0xC0000000

# process save area offsets
.set STATE_ESP,								0
.set STATE_EDI,								4
.set STATE_ESI,								8
.set STATE_EBP,								12
.set STATE_EFLAGS,						16
.set STATE_EBX,								20

# Multiboot constants
.set MULTIBOOT_PAGE_ALIGN,		1 << 0
.set MULTIBOOT_MEMORY_INFO,		1 << 1
.set MULTIBOOT_HEADER_MAGIC,	0x1BADB002
.set MULTIBOOT_HEADER_FLAGS,	MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
.set MULTIBOOT_CHECKSUM,			-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

# macro to build a default-isr-handler
.macro BUILD_DEF_ISR no
	.global isr\no
	isr\no:
	# interrupts are already disabled here since its a interrupt-gate, not a trap-gate
	pushl	$0														# error-code (no error here)
	pushl	$\no													# the interrupt-number
	jmp		isrCommon
.endm

# macro to build an error-isr-handler
.macro BUILD_ERR_ISR no
	.global isr\no
	isr\no:
	# interrupts are already disabled here since its a interrupt-gate, not a trap-gate
	# the error-code has already been pushed
	pushl	$\no													# the interrupt-number
	jmp		isrCommon
.endm

.section .text

# Multiboot header (needed to boot from GRUB)
.align 4
.long		MULTIBOOT_HEADER_MAGIC
.long		MULTIBOOT_HEADER_FLAGS
.long		MULTIBOOT_CHECKSUM

higherhalf:
	# from now the CPU will translate automatically every address
	# by adding the base 0x40000000

	cli																	# disable interrupts during startup
	mov		$kernelStack,%esp							# set up a new stack for our kernel
	mov		%esp,%ebp
	push	%ebp													# push ebp on the stack to ensure that the stack-trace works

	push	%eax													# push Multiboot Magicnumber onto the stack
  push	%ebx													# push address of Multiboot-Structure
  call	main													# jump to our C kernel (returns entry-point)
	add		$8,%esp												# remove args from stack

	# setup env for first task
	pushl	$0x23													# ss
	pushl	$USER_STACK - 4								# esp
	pushfl															# eflags
	mov		(%esp),%ecx
	or		$1 << 9,%ecx									# enable IF-flag
	and		$~((1 << 12) | (1 << 13)),%ecx	# set IOPL=0 (if CPL <= IOPL the user can change eflags..)
	mov		%ecx,(%esp)
	pushl	$0x1B													# cs
	push	%eax													# eip (entry-point)
	mov		$0x23,%eax										# set the value for the segment-registers
	mov		%eax,%ds											# reload segments
	mov		%eax,%es
	mov		%eax,%fs
	mov		$0x2B,%eax
	mov		%eax,%gs											# TLS segment
	iret																# jump to task and switch to user-mode

	# just a simple protection...
1:
	jmp		1b

# void gdt_flush(tGDTTable *gdt);
gdt_flush:
	mov		4(%esp),%eax									# load gdt-pointer into eax
	lgdt	(%eax)												# load gdt

	mov		$0x10,%eax										# set the value for the segment-registers
	mov		%eax,%ds											# reload segments
	mov		%eax,%es
	mov		%eax,%fs
	mov		%eax,%gs
	mov		%eax,%ss
	ljmp	$0x08,$2f											# reload code-segment via far-jump
2:
	ret																	# we're done

# void tss_load(u16 gdtOffset);
tss_load:
	ltr		4(%esp)												# load tss
	ret

# void util_halt(void);
util_halt:
	hlt

# void util_outByte(u16 port,u8 val);
util_outByte:
	mov		4(%esp),%dx										# load port
	mov		8(%esp),%al										# load value
	out		%al,%dx												# write to port
	ret

# void util_outWord(u16 port,u16 val);
util_outWord:
	mov		4(%esp),%dx										# load port
	mov		8(%esp),%ax										# load value
	out		%ax,%dx												# write to port
	ret

# void util_outWord(u16 port,u32 val);
util_outDWord:
	mov		4(%esp),%dx										# load port
	mov		8(%esp),%eax									# load value
	out		%eax,%dx											# write to port
	ret

# u8 util_inByte(u16 port);
util_inByte:
	mov		4(%esp),%dx										# load port
	in		%dx,%al												# read from port
	ret

# u16 util_inByte(u16 port);
util_inWord:
	mov		4(%esp),%dx										# load port
	in		%dx,%ax												# read from port
	ret

# u32 util_inByte(u16 port);
util_inDWord:
	mov		4(%esp),%dx										# load port
	in		%dx,%eax											# read from port
	ret

# u32 getStackFrameStart(void);
getStackFrameStart:
	mov		%ebp,%eax
	ret

# bool cpu_cpuidSupported(void);
cpu_cpuidSupported:
	pushfl
	pop		%eax													# load eflags into eax
	mov		%eax,%ecx											# make copy
	xor		$0x200000,%eax								# swap cpuid-bit
	and		$0x200000,%ecx								# isolate cpuid-bit
	push	%eax
	popfl																# store eflags
	pushfl
	pop		%eax													# load again to eax
	and		$0x200000,%eax								# isolate cpuid-bit
	xor		%ecx,%eax											# check whether the bit has been set
	shr		$21,%eax											# if so, return 1 (cpuid supported)
	ret

# u64 cpu_rdtsc(void);
cpu_rdtsc:
	rdtsc
	ret

# u32 cpu_getCR0(void);
cpu_getCR0:
	mov		%cr0,%eax
	ret

# void cpu_setCR0(u32 cr0);
cpu_setCR0:
	mov		4(%esp),%eax
	mov		%eax,%cr0
	ret

# u32 cpu_getCR2(void);
cpu_getCR2:
	mov		%cr2,%eax
	ret

# u32 cpu_getCR3(void);
cpu_getCR3:
	mov		%cr3,%eax
	ret

# u32 cpu_getCR4(void);
cpu_getCR4:
	mov		%cr4,%eax
	ret

# void cpu_setCR4(u32 cr4);
cpu_setCR4:
	mov		4(%esp),%eax
	mov		%eax,%cr4
	ret

# void cpu_getInfo(u32 code,u32 *a,u32 *b,u32 *c,u32 *d);
cpu_getInfo:
	push	%ebp
	mov		%esp,%ebp
	push	%ebx													# save ebx
	push	%esi													# save esi
	mov		8(%ebp),%eax									# load code into eax
	cpuid
	mov		12(%ebp),%esi									# store result in a,b,c and d
	mov		%eax,(%esi)
	mov		16(%ebp),%esi
	mov		%ebx,(%esi)
	mov		20(%ebp),%esi
	mov		%ecx,(%esi)
	mov		24(%ebp),%esi
	mov		%edx,(%esi)
	pop		%esi													# restore esi
	pop		%ebx													# restore ebx
	leave
	ret

# void cpu_getStrInfo(u32 code,char *res);
cpu_getStrInfo:
	push	%ebp
	mov		%esp,%ebp
	push	%ebx													# save ebx
	push	%esi													# save esi
	mov		8(%ebp),%eax									# load code into eax
	cpuid
	mov		12(%ebp),%esi									# load res into esi
	mov		%ebx,0(%esi)									# store result in res
	mov		%edx,4(%esi)
	mov		%ecx,8(%esi)
	pop		%esi													# restore esi
	pop		%ebx													# restore ebx
	leave
	ret

# void fpu_finit(void);
fpu_finit:
	finit
	ret

# void fpu_saveState(sFPUState *state);
fpu_saveState:
	mov			4(%esp),%eax
	fsave		(%eax)
	ret

# void fpu_restoreState(sFPUState *state);
fpu_restoreState:
	mov			4(%esp),%eax
	frstor	(%eax)
	ret

# bool thread_save(sThreadRegs *saveArea);
thread_save:
	push	%ebp
	mov		%esp,%ebp

	# save register
	mov		8(%ebp),%eax									# get saveArea
	mov		%ebx,STATE_EBX(%eax)
	mov		%esp,STATE_ESP(%eax)					# store esp
	mov		%edi,STATE_EDI(%eax)
	mov		%esi,STATE_ESI(%eax)
	mov		%ebp,STATE_EBP(%eax)
	pushfl															# load eflags
	popl	STATE_EFLAGS(%eax)						# store

	mov		$0,%eax												# return 0
	leave
	ret

# bool thread_resume(u32 pageDir,sThreadRegs *saveArea,u32 kstackFrame);
thread_resume:
	push	%ebp
	mov		%esp,%ebp

	mov		12(%ebp),%eax									# get saveArea
	mov		 8(%ebp),%edi									# get page-dir
	mov		16(%ebp),%esi									# get stack-frame

	test	%esi,%esi											# if stack-frame is 0 we just have one thread
	je		1f														# i.e. there can't be another stack-frame anyway

	# load new page-dir
	mov		%edi,%cr3											# set page-dir

	# exchange kernel-stack-frame
	mov		(KERNEL_STACK_PTE),%ecx
	and		$0x00000FFF,%ecx							# clear frame-number
	mov		%esi,%edx
	shl		$12,%edx
	or		%edx,%ecx											# set new frame-number
	mov		%ecx,(KERNEL_STACK_PTE)				# store

	# load page-dir again
1:
	mov		%edi,%cr3											# set page-dir

	# now restore registers
	mov		STATE_EDI(%eax),%edi
	mov		STATE_ESI(%eax),%esi
	mov		STATE_EBP(%eax),%ebp
	mov		STATE_ESP(%eax),%esp
	mov		STATE_EBX(%eax),%ebx
	pushl	STATE_EFLAGS(%eax)
	popfl																# load eflags

	mov		$1,%eax												# return 1
	leave
	ret

# void paging_enable(void);
paging_enable:
	mov		%cr0,%eax
	or		$1 << 31,%eax									# set bit for paging-enabled
	mov		%eax,%cr0											# now paging is enabled :)
	ret

# void paging_flushTLB(void);
paging_flushTLB:
	mov		%cr3,%eax
	mov		%eax,%cr3
	ret

# void paging_exchangePDir(u32 physAddr);
paging_exchangePDir:
	mov		4(%esp),%eax									# load page-dir-address
	mov		%eax,%cr3											# set page-dir
	ret

# bool intrpt_setEnabled(bool enabled);
intrpt_setEnabled:
	push	%ebp
	mov		%esp,%ebp
	mov		8(%esp),%eax
	shl		$9,%eax
	pushfl
	mov		(%esp),%ecx
	mov		%ecx,%eax
	and		$1 << 9,%eax									# extract IF-flag
	shr		$9,%eax
	or		%eax,%ecx											# set new value
	mov		%ecx,(%esp)
	popfl
	leave
	ret

# void intrpt_loadidt(tIDTPtr *idt);
intrpt_loadidt:
	mov		4(%esp),%eax									# load idt-address
	lidt	(%eax)												# load idt
	ret

# our ISRs
BUILD_DEF_ISR 0
BUILD_DEF_ISR 1
BUILD_DEF_ISR 2
BUILD_DEF_ISR 3
BUILD_DEF_ISR 4
BUILD_DEF_ISR 5
BUILD_DEF_ISR 6
BUILD_DEF_ISR 7
BUILD_ERR_ISR 8
BUILD_DEF_ISR 9
BUILD_ERR_ISR 10
BUILD_ERR_ISR 11
BUILD_ERR_ISR 12
BUILD_ERR_ISR 13
BUILD_ERR_ISR 14
BUILD_DEF_ISR 15
BUILD_DEF_ISR 16
BUILD_ERR_ISR 17
BUILD_DEF_ISR 18
BUILD_DEF_ISR 19
BUILD_DEF_ISR 20
BUILD_DEF_ISR 21
BUILD_DEF_ISR 22
BUILD_DEF_ISR 23
BUILD_DEF_ISR 24
BUILD_DEF_ISR 25
BUILD_DEF_ISR 26
BUILD_DEF_ISR 27
BUILD_DEF_ISR 28
BUILD_DEF_ISR 29
BUILD_DEF_ISR 30
BUILD_DEF_ISR 31
BUILD_DEF_ISR 32
BUILD_DEF_ISR 33
BUILD_DEF_ISR 34
BUILD_DEF_ISR 35
BUILD_DEF_ISR 36
BUILD_DEF_ISR 37
BUILD_DEF_ISR 38
BUILD_DEF_ISR 39
BUILD_DEF_ISR 40
BUILD_DEF_ISR 41
BUILD_DEF_ISR 42
BUILD_DEF_ISR 43
BUILD_DEF_ISR 44
BUILD_DEF_ISR 45
BUILD_DEF_ISR 46
BUILD_DEF_ISR 47
BUILD_DEF_ISR 48

# our null-handler for all other interrupts
.global isrNull
	isrNull:
	# interrupts are already disabled here since its a interrupt-gate, not a trap-gate
	pushl	$0														# error-code (no error here)
	pushl	$49														# the interrupt-number
	jmp		isrCommon

# the ISR for all interrupts
isrCommon:
	# save registers
	pusha
	push	%gs
	push	%fs
	push	%ds
	push	%es

	# set segment-registers for kernel
	# TODO just necessary for VM86-tasks because ds,es,fs and gs is set to 0 in this case
	mov		$0x23,%eax
	mov		%eax,%ds
	mov		%eax,%es
	mov		%eax,%fs
	mov		%eax,%gs

	# call c-routine
	push	%esp														# stack-frame
	mov		%esp,%eax
	push	%eax														# pointer to stack-frame
	call	intrpt_handler

	# remove arguments from stack
	add		$8,%esp

	# restore registers
	pop		%es
	pop		%ds
	pop		%fs
	pop		%gs
	popa

	# remove error-code and interrupt-number from stack
	add		$8,%esp

	# return
	iret

.section .bss

# our temporary kernel stack
.rept TMP_STACK_SIZE
	.byte 0
.endr
kernelStack:
