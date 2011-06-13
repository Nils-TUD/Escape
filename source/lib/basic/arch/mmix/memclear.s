/**
 * $Id: memcpy.s 900 2011-06-02 20:18:17Z nasmussen $
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

.global memclear

dest	IS	$0
count	IS	$1
tmp		IS	$2
tmp2	IS	$3
off		IS	$4
rem		IS	$5
val		IS	$6

# void memclear(void *dest,size_t count)
memclear:
	SETL	off,0							# offset
	SET		rem,count						# remaining
	SETL	val,0							# the value to store
	AND		tmp,dest,7
	BZ		tmp,3f							# already word-aligned?
2:
	# ok, word-align dest
	NEGU	tmp2,8,tmp						# remaining = 8 - (addr & 7)
	CMPU	tmp,rem,tmp2
	BN		tmp,5f							# less than remaining?
	SET		rem,tmp2						# yes, so use tmp2
	JMP		5f
4:
	STBU	val,dest,off
	ADDU	off,off,1
	SUBU	rem,rem,1
5:
	BNZ		rem,4b							# stop if rem == 0
3:
	# now dest is word aligned
	# first, clear with loop-unrolling
	SUBU	count,count,off					# count = number of remaining bytes
	SET		rem,count
	ANDNL	rem,#003F						# align it to 8*8 bytes
	SUBU	count,count,rem					# the remaining bytes after this loop
	JMP		2f
3:
	STCO	0,dest,0						# word 1
	STCO	0,dest,8						# word 2
	STCO	0,dest,16						# word 3
	STCO	0,dest,24						# word 4
	STCO	0,dest,32						# word 5
	STCO	0,dest,40						# word 6
	STCO	0,dest,48						# word 7
	STCO	0,dest,56						# word 8
	ADDU	dest,dest,64
	SUBU	rem,rem,64
2:
	BNZ		rem,3b							# stop if rem == 0
	# now copy the remaining words
	SET		rem,count						# the remaining bytes
	ANDNL	rem,#0007						# word-align it
	SUBU	count,count,rem					# the remaining bytes after this loop
	JMP		2f
3:
	STCO	0,dest,off
	ADDU	off,off,8
	SUBU	rem,rem,8
2:
	BNZ		rem,3b							# stop if rem == 0
	# maybe, there are some bytes left to copy
	OR		rem,count,count
	JMP		1f
	# default version: byte copy
2:
	STBU	val,dest,off
	ADDU	off,off,1
	SUBU	rem,rem,1
1:
	BNZ		rem,2b							# stop if rem == 0
3:
	POP		0,0
