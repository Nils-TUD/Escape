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

.global cpu_cpuidSupported
.global cpu_getInfo
.global cpu_getStrInfo

// bool CPU::cpuidSupported();
cpu_cpuidSupported:
	pushfl
	pop		%eax					// load eflags into eax
	mov		%eax,%ecx				// make copy
	xor		$0x200000,%eax			// swap cpuid-bit
	and		$0x200000,%ecx			// isolate cpuid-bit
	push	%eax
	popfl							// store eflags
	pushfl
	pop		%eax					// load again to eax
	and		$0x200000,%eax			// isolate cpuid-bit
	xor		%ecx,%eax				// check whether the bit has been set
	shr		$21,%eax				// if so, return 1 (cpuid supported)
	ret

// void CPU::getInfo(uint32_t code,uint32_t *a,uint32_t *b,uint32_t *c,uint32_t *d);
cpu_getInfo:
	push	%ebp
	mov		%esp,%ebp
	push	%ebx					// save ebx, ecx and edx; cpuid overwrites them
	push	%ecx
	push	%edx
	cpuid							// code is already in eax
	pop		%edi
	mov		%eax,(%edi)				// store result in a,b,c and d
	pop		%edi
	mov		%ebx,(%edi)
	mov		8(%ebp),%edi
	mov		%ecx,(%edi)
	mov		12(%ebp),%edi
	mov		%edx,(%edi)
	pop		%ebx					// restore ebx
	leave
	ret

// void CPU::getStrInfo(uint32_t code,char *res);
cpu_getStrInfo:
	push	%ebp
	mov		%esp,%ebp
	push	%ebx					// save ebx
	push	%edx					// save edx
	cpuid							// code is already in eax
	pop		%edi
	mov		%ebx,0(%edi)			// store result in res
	mov		%edx,4(%edi)
	mov		%ecx,8(%edi)
	pop		%ebx					// restore ebx
	leave
	ret
