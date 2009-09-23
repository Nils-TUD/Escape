;
; $Id$
; Copyright (C) 2008 - 2009 Nils Asmussen
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
;

[BITS 32]      												; 32 bit code

; exports
[global loader]
[global gdt_flush]
[global tss_load]
[global util_outByte]
[global util_inByte]
[global KernelStart]
[global util_halt]
[global intrpt_setEnabled]
[global intrpt_loadidt]
[global paging_enable]
[global paging_flushAddr]
[global paging_flushTLB]
[global paging_exchangePDir]
[global cpu_cpuidSupported]
[global cpu_rdtsc]
[global cpu_getCR0]
[global cpu_setCR0]
[global cpu_getCR2]
[global cpu_getCR3]
[global cpu_getCR4]
[global cpu_setCR4]
[global fpu_finit]
[global fpu_saveState]
[global fpu_restoreState]
[global thread_save]
[global thread_resume]
[global getStackFrameStart]
[global kernelStack]

; imports
[extern main]
[extern intrpt_handler]
[extern entryPoint]

; Multiboot constants
MULTIBOOT_PAGE_ALIGN		equ 1<<0
MULTIBOOT_MEMORY_INFO		equ 1<<1
MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS	equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MULTIBOOT_CHECKSUM			equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

; general constants
; TODO better way which uses the defines from paging.h?
PAGE_SIZE								equ 4096
KERNEL_STACK						equ 0xFF7FE000
KERNEL_STACK_PTE				equ 0xFFFFDFF8
TMP_STACK_SIZE					equ PAGE_SIZE
USER_STACK							equ 0xC0000000

; process save area offsets
STATE_ESP								equ 0
STATE_EDI								equ 4
STATE_ESI								equ 8
STATE_EBP								equ 12
STATE_EFLAGS						equ 16
STATE_EBX								equ 20

; TODO consider callee-save-registers!!

; macro to build a default-isr-handler
%macro BUILD_DEF_ISR 1
	[global isr%1]
	isr%1:
	cli																	; disable interrupts
	push	0															; error-code (no error here)
	push	dword %1											; the interrupt-number
	jmp		isrCommon
%endmacro

; macro to build an error-isr-handler
%macro BUILD_ERR_ISR 1
	[global isr%1]
	isr%1:
	cli																	; disable interrupts
	; the error-code has already been pushed
	push	dword %1											; the interrupt-number
	jmp		isrCommon
%endmacro

[section .setup]

; Multiboot header (needed to boot from GRUB)
ALIGN 4
KernelStart:
multiboot_header:
	dd		MULTIBOOT_HEADER_MAGIC
	dd		MULTIBOOT_HEADER_FLAGS
	dd		MULTIBOOT_CHECKSUM

; the kernel entry point
loader:
	; here is the trick: we load a GDT with a base address
	; of 0x40000000 for the code (0x08) and data (0x10) segments
	lgdt	[setupGDT]
	mov		ax,0x10
	mov		ds,ax
	mov		es,ax
	mov		fs,ax
	mov		gs,ax
	mov		ss,ax

	; jump to the higher half kernel
	jmp		0x08:higherhalf

[section .text]

higherhalf:
	; from now the CPU will translate automatically every address
	; by adding the base 0x40000000

	cli																	; disable interrupts during startup
	mov		esp,kernelStack								; set up a new stack for our kernel
	mov		ebp,esp
	push	ebp														; push ebp on the stack to ensure that the stack-trace works

	push	eax														; push Multiboot Magicnumber onto the stack
  push	ebx														; push address of Multiboot-Structure
  call	main													; jump to our C kernel (returns entry-point)
	add		esp,8													; remove args from stack

	; setup env for first task
	push	DWORD 0x23										; ss
	push	USER_STACK - 4								; esp
	pushfd															; eflags
	mov		ebx,[esp]
	or		ebx,1 << 9										; enable IF-flag
	mov		[esp],ebx
	push	DWORD 0x1B										; cs
	push	eax														; eip (entry-point)
	mov		eax,0x23											; set the value for the segment-registers
	mov		ds,eax												; reload segments
	mov		es,eax
	mov		fs,eax
	mov		gs,eax
	iret																; jump to task and switch to user-mode

	; just a simple protection...
	jmp		$

; void gdt_flush(tGDTTable *gdt);
gdt_flush:
	mov		eax,[esp+4]										; load gdt-pointer into eax
	lgdt	[eax]													; load gdt

	mov		eax,0x10											; set the value for the segment-registers
	mov		ds,eax												; reload segments
	mov		es,eax
	mov		fs,eax
	mov		gs,eax
	mov		ss,eax
	jmp		0x08:gdt_flush_ljmp						; reload code-segment via far-jump
gdt_flush_ljmp:
	ret																	; we're done

; void tss_load(u16 gdtOffset);
tss_load:
	ltr		[esp+4]												; load tss
	ret

; void util_halt(void);
util_halt:
	hlt

; void util_outByte(u16 port,u8 val);
util_outByte:
	mov		dx,[esp+4]										; load port
	mov		al,[esp+8]										; load value
	out		dx,al													; write to port
	ret

; u8 util_inByte(u16 port);
util_inByte:
	mov		dx,[esp+4]										; load port
	in		al,dx													; read from port
	ret

; u32 getStackFrameStart(void);
getStackFrameStart:
	mov		eax,ebp
	ret

; bool cpu_cpuidSupported(void);
cpu_cpuidSupported:
	pushfd
	pop		eax														; load eflags into eax
	mov		ebx,eax												; make copy
	xor		eax,0x200000									; swap cpuid-bit
	and		ebx,0x200000									; isolate cpuid-bit
	push	eax
	popfd																; store eflags
	pushfd
	pop		eax														; load again to eax
	and		eax,0x200000									; isolate cpuid-bit
	xor		eax,ebx												; check wether the bit has been set
	shr		eax,21												; if so, return 1 (cpuid supported)
	ret

; u64 cpu_rdtsc(void);
cpu_rdtsc:
	rdtsc
	ret

; u32 cpu_getCR0(void);
cpu_getCR0:
	mov		eax,cr0
	ret

; void cpu_setCR0(u32 cr0);
cpu_setCR0:
	mov		eax,[esp + 4]
	mov		cr0,eax
	ret

; u32 cpu_getCR2(void);
cpu_getCR2:
	mov		eax,cr2
	ret

; u32 cpu_getCR3(void);
cpu_getCR3:
	mov		eax,cr3
	ret

; u32 cpu_getCR4(void);
cpu_getCR4:
	mov		eax,cr4
	ret

; void cpu_setCR4(u32 cr4);
cpu_setCR4:
	mov		eax,[esp + 4]
	mov		cr4,eax
	ret

; void fpu_finit(void);
fpu_finit:
	finit
	ret

; void fpu_saveState(sFPUState *state);
fpu_saveState:
	mov			eax,[esp + 4]
	fsave		[eax]
	ret

; void fpu_restoreState(sFPUState *state);
fpu_restoreState:
	mov			eax,[esp + 4]
	frstor	[eax]
	ret

; bool thread_save(sThreadRegs *saveArea);
thread_save:
	push	ebp
	mov		ebp,esp
	sub		esp,4

	; save register
	mov		eax,[ebp + 8]									; get saveArea
	mov		[eax + STATE_EBX],ebx
	mov		[eax + STATE_ESP],esp					; store esp
	mov		[eax + STATE_EDI],edi
	mov		[eax + STATE_ESI],esi
	mov		[eax + STATE_EBP],ebp
	pushfd															; load eflags
	pop		DWORD [eax + STATE_EFLAGS]		; store

	mov		eax,0													; return 0
	leave
	ret

; bool thread_resume(u32 pageDir,sThreadRegs *saveArea,u32 kstackFrame);
thread_resume:
	push	ebp
	mov		ebp,esp

	; restore register
	mov		eax,[ebp + 12]								; get saveArea
	mov		edi,[ebp + 8]									; get page-dir
	mov		esi,[ebp + 16]								; get stack-frame

	; load new page-dir
	mov		cr3,edi												; set page-dir

	; exchange kernel-stack-frame
	mov		ecx,[KERNEL_STACK_PTE]
	and		ecx,0x00000FFF								; clear frame-number
	mov		edx,esi
	shl		edx,12
	or		ecx,edx												; set new frame-number
	mov		[KERNEL_STACK_PTE],ecx				; store

	; load page-dir again
	mov		cr3,edi												; set page-dir

	; now restore registers
	mov		edi,[eax + STATE_EDI]
	mov		esi,[eax + STATE_ESI]
	mov		ebp,[eax + STATE_EBP]
	mov		esp,[eax + STATE_ESP]
	mov		ebx,[eax + STATE_EBX]
	push	DWORD [eax + STATE_EFLAGS]
	popfd																; load eflags

	mov		eax,1													; return 1
	leave
	ret

; void paging_enable(void);
paging_enable:
	mov		eax,cr0
	or		eax,1 << 31										; set bit for paging-enabled
	mov		cr0,eax												; now paging is enabled :)
	ret

; NOTE: supported for >= Intel486
; void paging_flushAddr(u32 address);
paging_flushAddr:
	invlpg	[esp + 4]
	ret

; void paging_flushTLB(void);
paging_flushTLB:
	mov		eax,cr3
	mov		cr3,eax
	ret

; void paging_exchangePDir(u32 physAddr);
paging_exchangePDir:
	mov		eax,[esp+4]										; load page-dir-address
	mov		cr3,eax												; set page-dir
	ret

; bool intrpt_setEnabled(bool enabled);
intrpt_setEnabled:
	push	ebp
	mov		ebp,esp
	mov		ebx,[esp + 8]
	shl		ebx,9
	pushfd
	mov		ecx,[esp]
	mov		eax,ecx
	and		eax,1 << 9										; extract IF-flag
	shr		eax,9
	or		ecx,ebx												; set new value
	mov		[esp],ecx
	popfd
	leave
	ret

; void intrpt_loadidt(tIDTPtr *idt);
intrpt_loadidt:
	mov		eax,[esp+4]										; load idt-address
	lidt	[eax]													; load idt
	ret

; our ISRs
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

; our null-handler for all other interrupts
[global isrNull]
	isrNull:
	cli																	; disable interrupts
	push	0															; error-code (no error here)
	push	dword 49											; the interrupt-number
	jmp		isrCommon

; the ISR for all interrupts
isrCommon:
	; save registers
	push	eax
	push	ebx
	push	ecx
	push	edx
	push	ebp
	push	esi
	push	edi
	push	gs
	push	fs
	push	ds
	push	es

	; call c-routine
	push	esp														; pointer to the "beginning" of the stack
	call	intrpt_handler

	; remove argument from stack
	pop		esp
	;add		esp,4

	; restore registers
	pop		es
	pop		ds
	pop		fs
	pop		gs
	pop		edi
	pop		esi
	pop		ebp
	pop		edx
	pop		ecx
	pop		ebx
	pop		eax

	; remove error-code and interrupt-number from stack
	add		esp,8

	; enable interrupts and return
	sti
	iret

[section .setup]

; our GDT for the setup-process
setupGDT:
	; GDT size
	dw		setupGDTEntries - setupGDTEntriesEnd - 1
	; Pointer to entries
	dd		setupGDTEntries

; the GDT-entries
setupGDTEntries:
	; null gate
	dd 0,0

	; code selector 0x08: base 0x40000000, limit 0xFFFFFFFF, type 0x9A, granularity 0xCF
	db 0xFF, 0xFF, 0, 0, 0, 10011010b, 11001111b, 0x40

	; data selector 0x10: base 0x40000000, limit 0xFFFFFFFF, type 0x92, granularity 0xCF
	db 0xFF, 0xFF, 0, 0, 0, 10010010b, 11001111b, 0x40
setupGDTEntriesEnd:

[section .bss]

resb TMP_STACK_SIZE
kernelStack:
	; our temporary kernel stack
