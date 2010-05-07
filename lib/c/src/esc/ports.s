;
; $Id: ports.s 650 2010-05-06 19:05:10Z nasmussen $
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

[global inByte:function]
[global inWord:function]
[global inDWord:function]
[global outByte:function]
[global outWord:function]
[global outDWord:function]

; u8 inByte(u16 port)
inByte:
	mov		dx,[esp + 4]									; load port
	in		al,dx													; read from port
	ret

; u16 inWord(u16 port)
inWord:
	mov		dx,[esp + 4]									; load port
	in		ax,dx													; read from port
	ret

; u32 inDWord(u16 port)
inDWord:
	mov		dx,[esp + 4]									; load port
	in		eax,dx												; read from port
	ret

; void outByte(u16 port,u8 val)
outByte:
	mov		dx,[esp + 4]									; load port
	mov		al,[esp + 8]									; load value
	out		dx,al													; write to port
	ret

; void outWord(u16 port,u16 val)
outWord:
	mov		dx,[esp + 4]									; load port
	mov		ax,[esp + 8]									; load value
	out		dx,ax													; write to port
	ret

; void outDWord(u16 port,u32 val)
outDWord:
	mov		dx,[esp + 4]									; load port
	mov		eax,[esp + 8]									; load value
	out		dx,eax												; write to port
	ret
