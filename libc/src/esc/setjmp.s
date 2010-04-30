;
; $Id: ports.s 624 2010-04-19 21:52:37Z nasmussen $
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

[BITS 32]

[global setjmp:function]
[global longjmp:function]

; s32 setjmp(sJumpEnv *env);
setjmp:
	push	ebp
	mov		ebp,esp

	; save regs (callee-save)
	mov		eax,[ebp + 8]									; get env
	mov		[eax + 0],ebx
	mov		[eax + 4],esp
	mov		[eax + 8],edi
	mov		[eax + 12],esi
	mov		ecx,[ebp]											; store ebp of the caller stack-frame
	mov		[eax + 16],ecx
	pushfd															; load eflags
	pop		DWORD [eax + 20]							; store
	mov		ecx,[ebp + 4]									; store return-address
	mov		[eax + 24],ecx

	mov		eax,0													; return 0
	leave
	ret

; s32 longjmp(sJumpEnv *env,s32 val);
longjmp:
	push	ebp
	mov		ebp,esp
	mov		ecx,[ebp + 12]								; get val

	; restore registers (callee-save)
	mov		eax,[ebp + 8]									; get env
	mov		edi,[eax + 8]
	mov		esi,[eax + 12]
	mov		ebp,[eax + 16]
	mov		esp,[eax + 4]
	add		esp,4													; undo 'push ebp'
	mov		ebx,[eax + 0]
	push	DWORD [eax + 20]							; get eflags
	popfd																; restore
	mov		eax,[eax + 24]								; get return-address
	mov		[esp],eax											; set return-address

	mov		eax,ecx												; return val
	ret																	; no leave here because we've already restored the
																			; ebp of the caller stack-frame
