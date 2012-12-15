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
.global cpu_getMSR
.global cpu_setMSR

# bool cpu_cpuidSupported(void);
cpu_cpuidSupported:
	pushfl
	pop		%eax					# load eflags into eax
	mov		%eax,%ecx				# make copy
	xor		$0x200000,%eax			# swap cpuid-bit
	and		$0x200000,%ecx			# isolate cpuid-bit
	push	%eax
	popfl							# store eflags
	pushfl
	pop		%eax					# load again to eax
	and		$0x200000,%eax			# isolate cpuid-bit
	xor		%ecx,%eax				# check whether the bit has been set
	shr		$21,%eax				# if so, return 1 (cpuid supported)
	ret

# uint64_t cpu_rdtsc(void);
cpu_rdtsc:
	rdtsc
	ret

# uint32_t cpu_getCR0(void);
cpu_getCR0:
	mov		%cr0,%eax
	ret

# void cpu_setCR0(uint32_t cr0);
cpu_setCR0:
	mov		4(%esp),%eax
	mov		%eax,%cr0
	ret

# uint32_t cpu_getCR2(void);
cpu_getCR2:
	mov		%cr2,%eax
	ret

# uint32_t cpu_getCR3(void);
cpu_getCR3:
	mov		%cr3,%eax
	ret

# uint32_t cpu_getCR4(void);
cpu_getCR4:
	mov		%cr4,%eax
	ret

# void cpu_setCR4(uint32_t cr4);
cpu_setCR4:
	mov		4(%esp),%eax
	mov		%eax,%cr4
	ret

# uint64_t cpu_getMSR(uint32_t msr)
cpu_getMSR:
	mov		4(%esp),%ecx
	rdmsr
	ret

# void cpu_setMSR(uint32_t msr,uint64_t value)
cpu_setMSR:
	mov		4(%esp),%ecx
	mov		8(%esp),%eax
	mov		12(%esp),%edx
	wrmsr
	ret

# void cpu_getInfo(uint32_t code,uint32_t *a,uint32_t *b,uint32_t *c,uint32_t *d);
cpu_getInfo:
	push	%ebp
	mov		%esp,%ebp
	push	%ebx					# save ebx
	push	%esi					# save esi
	mov		8(%ebp),%eax			# load code into eax
	cpuid
	mov		12(%ebp),%esi			# store result in a,b,c and d
	mov		%eax,(%esi)
	mov		16(%ebp),%esi
	mov		%ebx,(%esi)
	mov		20(%ebp),%esi
	mov		%ecx,(%esi)
	mov		24(%ebp),%esi
	mov		%edx,(%esi)
	pop		%esi					# restore esi
	pop		%ebx					# restore ebx
	leave
	ret

# void cpu_getStrInfo(uint32_t code,char *res);
cpu_getStrInfo:
	push	%ebp
	mov		%esp,%ebp
	push	%ebx					# save ebx
	push	%esi					# save esi
	mov		8(%ebp),%eax			# load code into eax
	cpuid
	mov		12(%ebp),%esi			# load res into esi
	mov		%ebx,0(%esi)			# store result in res
	mov		%edx,4(%esi)
	mov		%ecx,8(%esi)
	pop		%esi					# restore esi
	pop		%ebx					# restore ebx
	leave
	ret
