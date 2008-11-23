;
; @version		$Id$
; @author		Nils Asmussen <nils@script-solution.de>
; @copyright	2008 Nils Asmussen
;

[BITS 32]      												; 32 bit code

; exports
[global loader]
[global gdt_flush]
[global tss_load]
[global outb]
[global inb]
[global KernelStart]
[global halt]
[global intrpt_setEnabled]
[global intrpt_loadidt]
[global paging_enable]
[global paging_flushAddr]
[global paging_flushTLB]
[global paging_exchangePDir]
[global cpu_rdtsc]
[global cpu_getCR0]
[global cpu_getCR1]
[global cpu_getCR2]
[global cpu_getCR3]
[global cpu_getCR4]
[global proc_save]
[global proc_resume]
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
KERNEL_STACK_SIZE				equ PAGE_SIZE
KERNEL_STACK						equ (0xFFFFFFFF - 0x400000 - KERNEL_STACK_SIZE) + 1
USER_STACK							equ 0xC0000000

; process save area offsets
STATE_ESP								equ 0
STATE_EDI								equ 4
STATE_ESI								equ 8
STATE_EBP								equ 12
STATE_EFLAGS						equ 16

; TODO consider callee-save-registers!!

; macro to build a default-isr-handler
%macro BUILD_DEF_ISR 1
	[global isr%1]
	isr%1:
	cli																	; disable interrupts
	; init kernel-stack
	isr%1StackSet:
	push	0															; error-code (no error here)
	push	dword %1											; the interrupt-number
	jmp		isrCommon
%endmacro

; macro to build an error-isr-handler
%macro BUILD_ERR_ISR 1
	[global isr%1]
	isr%1:
	cli																	; disable interrupts
	; init kernel-stack
	isr%1StackSet:
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

; void halt(void);
halt:
	hlt

; void outb(u16 port,u8 val);
outb:
	mov		dx,[esp+4]										; load port
	mov		al,[esp+8]										; load value
	out		dx,al													; write to port
	ret

; u8 inb(u16 port);
inb:
	mov		dx,[esp+4]										; load port
	in		al,dx													; read from port
	ret

; u32 getStackFrameStart(void);
getStackFrameStart:
	mov		eax,ebp
	ret

; u64 cpu_rdtsc(void);
cpu_rdtsc:
	rdtsc
	ret

; u32 cpu_getCR0(void);
cpu_getCR0:
	mov		eax,cr0
	ret

; u32 cpu_getCR1(void);
cpu_getCR1:
	;mov		eax,cr1
	mov		eax,0
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

; bool proc_save(sProcSave *saveArea);
proc_save:
	push	ebp
	mov		ebp,esp
	sub		esp,4
	; save eax & ensure interrupts are disabled
	pushfd
	mov		eax,[esp]
	and		eax,1 << 9
	mov		[ebp-4],eax
	cli
	add		esp,4

	; save register
	mov		eax,[ebp + 8]									; get saveArea
	mov		[eax + STATE_ESP],esp					; store esp
	mov		[eax + STATE_EDI],edi
	mov		[eax + STATE_ESI],esi
	mov		[eax + STATE_EBP],ebp
	pushfd															; load eflags
	pop		DWORD [eax + STATE_EFLAGS]		; store

	; restore interrupt-state
	mov		eax,[ebp-4]
	cmp		eax,eax
	jz		proc_saveIFD
	sti
proc_saveIFD:
	mov		eax,0													; return 0
	leave
	ret

; bool proc_resume(u32 pageDir,sProcSave *saveArea);
proc_resume:
	push	ebp
	mov		ebp,esp
	; ensure interrupts are disabled
	pushfd
	mov		eax,[esp]
	and		eax,1 << 9
	cli
	add		esp,4

	; restore register
	mov		eax,[ebp + 12]								; get saveArea
	mov		edi,[eax + STATE_EDI]
	mov		esi,[eax + STATE_ESI]
	mov		ebp,[eax + STATE_EBP]
	push	DWORD [eax + STATE_EFLAGS]
	popfd																; load eflags

	; load new page-dir
	mov		ecx,[esp + 8]									; load page-dir-address
	mov		cr3,ecx												; set page-dir

	; now load esp
	mov		esp,[eax + STATE_ESP]

	; restore interrupt-state
	mov		ecx,[eax + STATE_EFLAGS]
	cmp		ecx,ecx
	jz		proc_resumeIFRes
	sti
proc_resumeIFRes:
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
BUILD_DEF_ISR 49
BUILD_DEF_ISR 50
BUILD_DEF_ISR 51
BUILD_DEF_ISR 52
BUILD_DEF_ISR 53
BUILD_DEF_ISR 54
BUILD_DEF_ISR 55
BUILD_DEF_ISR 56
BUILD_DEF_ISR 57
BUILD_DEF_ISR 58
BUILD_DEF_ISR 59
BUILD_DEF_ISR 60
BUILD_DEF_ISR 61
BUILD_DEF_ISR 62
BUILD_DEF_ISR 63
BUILD_DEF_ISR 64
BUILD_DEF_ISR 65
BUILD_DEF_ISR 66
BUILD_DEF_ISR 67
BUILD_DEF_ISR 68
BUILD_DEF_ISR 69
BUILD_DEF_ISR 70
BUILD_DEF_ISR 71
BUILD_DEF_ISR 72
BUILD_DEF_ISR 73
BUILD_DEF_ISR 74
BUILD_DEF_ISR 75
BUILD_DEF_ISR 76
BUILD_DEF_ISR 77
BUILD_DEF_ISR 78
BUILD_DEF_ISR 79
BUILD_DEF_ISR 80
BUILD_DEF_ISR 81
BUILD_DEF_ISR 82
BUILD_DEF_ISR 83
BUILD_DEF_ISR 84
BUILD_DEF_ISR 85
BUILD_DEF_ISR 86
BUILD_DEF_ISR 87
BUILD_DEF_ISR 88
BUILD_DEF_ISR 89
BUILD_DEF_ISR 90
BUILD_DEF_ISR 91
BUILD_DEF_ISR 92
BUILD_DEF_ISR 93
BUILD_DEF_ISR 94
BUILD_DEF_ISR 95
BUILD_DEF_ISR 96
BUILD_DEF_ISR 97
BUILD_DEF_ISR 98
BUILD_DEF_ISR 99
BUILD_DEF_ISR 100
BUILD_DEF_ISR 101
BUILD_DEF_ISR 102
BUILD_DEF_ISR 103
BUILD_DEF_ISR 104
BUILD_DEF_ISR 105
BUILD_DEF_ISR 106
BUILD_DEF_ISR 107
BUILD_DEF_ISR 108
BUILD_DEF_ISR 109
BUILD_DEF_ISR 110
BUILD_DEF_ISR 111
BUILD_DEF_ISR 112
BUILD_DEF_ISR 113
BUILD_DEF_ISR 114
BUILD_DEF_ISR 115
BUILD_DEF_ISR 116
BUILD_DEF_ISR 117
BUILD_DEF_ISR 118
BUILD_DEF_ISR 119
BUILD_DEF_ISR 120
BUILD_DEF_ISR 121
BUILD_DEF_ISR 122
BUILD_DEF_ISR 123
BUILD_DEF_ISR 124
BUILD_DEF_ISR 125
BUILD_DEF_ISR 126
BUILD_DEF_ISR 127
BUILD_DEF_ISR 128
BUILD_DEF_ISR 129
BUILD_DEF_ISR 130
BUILD_DEF_ISR 131
BUILD_DEF_ISR 132
BUILD_DEF_ISR 133
BUILD_DEF_ISR 134
BUILD_DEF_ISR 135
BUILD_DEF_ISR 136
BUILD_DEF_ISR 137
BUILD_DEF_ISR 138
BUILD_DEF_ISR 139
BUILD_DEF_ISR 140
BUILD_DEF_ISR 141
BUILD_DEF_ISR 142
BUILD_DEF_ISR 143
BUILD_DEF_ISR 144
BUILD_DEF_ISR 145
BUILD_DEF_ISR 146
BUILD_DEF_ISR 147
BUILD_DEF_ISR 148
BUILD_DEF_ISR 149
BUILD_DEF_ISR 150
BUILD_DEF_ISR 151
BUILD_DEF_ISR 152
BUILD_DEF_ISR 153
BUILD_DEF_ISR 154
BUILD_DEF_ISR 155
BUILD_DEF_ISR 156
BUILD_DEF_ISR 157
BUILD_DEF_ISR 158
BUILD_DEF_ISR 159
BUILD_DEF_ISR 160
BUILD_DEF_ISR 161
BUILD_DEF_ISR 162
BUILD_DEF_ISR 163
BUILD_DEF_ISR 164
BUILD_DEF_ISR 165
BUILD_DEF_ISR 166
BUILD_DEF_ISR 167
BUILD_DEF_ISR 168
BUILD_DEF_ISR 169
BUILD_DEF_ISR 170
BUILD_DEF_ISR 171
BUILD_DEF_ISR 172
BUILD_DEF_ISR 173
BUILD_DEF_ISR 174
BUILD_DEF_ISR 175
BUILD_DEF_ISR 176
BUILD_DEF_ISR 177
BUILD_DEF_ISR 178
BUILD_DEF_ISR 179
BUILD_DEF_ISR 180
BUILD_DEF_ISR 181
BUILD_DEF_ISR 182
BUILD_DEF_ISR 183
BUILD_DEF_ISR 184
BUILD_DEF_ISR 185
BUILD_DEF_ISR 186
BUILD_DEF_ISR 187
BUILD_DEF_ISR 188
BUILD_DEF_ISR 189
BUILD_DEF_ISR 190
BUILD_DEF_ISR 191
BUILD_DEF_ISR 192
BUILD_DEF_ISR 193
BUILD_DEF_ISR 194
BUILD_DEF_ISR 195
BUILD_DEF_ISR 196
BUILD_DEF_ISR 197
BUILD_DEF_ISR 198
BUILD_DEF_ISR 199
BUILD_DEF_ISR 200
BUILD_DEF_ISR 201
BUILD_DEF_ISR 202
BUILD_DEF_ISR 203
BUILD_DEF_ISR 204
BUILD_DEF_ISR 205
BUILD_DEF_ISR 206
BUILD_DEF_ISR 207
BUILD_DEF_ISR 208
BUILD_DEF_ISR 209
BUILD_DEF_ISR 210
BUILD_DEF_ISR 211
BUILD_DEF_ISR 212
BUILD_DEF_ISR 213
BUILD_DEF_ISR 214
BUILD_DEF_ISR 215
BUILD_DEF_ISR 216
BUILD_DEF_ISR 217
BUILD_DEF_ISR 218
BUILD_DEF_ISR 219
BUILD_DEF_ISR 220
BUILD_DEF_ISR 221
BUILD_DEF_ISR 222
BUILD_DEF_ISR 223
BUILD_DEF_ISR 224
BUILD_DEF_ISR 225
BUILD_DEF_ISR 226
BUILD_DEF_ISR 227
BUILD_DEF_ISR 228
BUILD_DEF_ISR 229
BUILD_DEF_ISR 230
BUILD_DEF_ISR 231
BUILD_DEF_ISR 232
BUILD_DEF_ISR 233
BUILD_DEF_ISR 234
BUILD_DEF_ISR 235
BUILD_DEF_ISR 236
BUILD_DEF_ISR 237
BUILD_DEF_ISR 238
BUILD_DEF_ISR 239
BUILD_DEF_ISR 240
BUILD_DEF_ISR 241
BUILD_DEF_ISR 242
BUILD_DEF_ISR 243
BUILD_DEF_ISR 244
BUILD_DEF_ISR 245
BUILD_DEF_ISR 246
BUILD_DEF_ISR 247
BUILD_DEF_ISR 248
BUILD_DEF_ISR 249
BUILD_DEF_ISR 250
BUILD_DEF_ISR 251
BUILD_DEF_ISR 252
BUILD_DEF_ISR 253
BUILD_DEF_ISR 254
BUILD_DEF_ISR 255

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

; TODO size ok?
resb 0x1000
kernelStack:
	; our temporary kernel stack
