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

.global memcpy

res		IS	$0
src		IS	$1
len		IS	$2
tmp		IS	$3
tmp2	IS	$4
off		IS	$5
rem		IS	$6
dest	IS	$7

# void *memcpy(void *dest,const void *src,size_t len)
memcpy:
	SETL	off,0							# offset
	SET		rem,len							# remaining
	SET		dest,res						# leave $0 unchanged
	AND		tmp,dest,7
	AND		tmp2,src,7
	CMPU	tmp2,tmp,tmp2
	BNZ		tmp2,1f							# not the same alignment? then a byte-copy is required
	BZ		tmp,3f							# already word-aligned?
2:
	# ok, word-align them
	NEGU	tmp2,8,tmp						# remaining = 8 - (addr & 7)
	CMPU	tmp,rem,tmp2
	BN		tmp,5f							# less than remaining?
	SET		rem,tmp2						# yes, so use $4
	JMP		5f
4:
	LDBU	tmp,src,off
	STBU	tmp,dest,off
	ADDU	off,off,1
	SUBU	rem,rem,1
5:
	BNZ		rem,4b							# stop if rem == 0
3:
	# now they are word aligned
	# first, copy with loop-unrolling
	SUBU	len,len,off						# len = number of remaining bytes
	SET		rem,len
	ANDNL	rem,#003F						# align it to 8*8 bytes
	SUBU	len,len,rem						# the remaining bytes after this loop
	JMP		2f
3:
	LDOU	tmp,src,0
	STOU	tmp,dest,0						# word 1
	LDOU	tmp,src,8
	STOU	tmp,dest,8						# word 2
	LDOU	tmp,src,16
	STOU	tmp,dest,16						# word 3
	LDOU	tmp,src,24
	STOU	tmp,dest,24						# word 4
	LDOU	tmp,src,32
	STOU	tmp,dest,32						# word 5
	LDOU	tmp,src,40
	STOU	tmp,dest,40						# word 6
	LDOU	tmp,src,48
	STOU	tmp,dest,48						# word 7
	LDOU	tmp,src,56
	STOU	tmp,dest,56						# word 8
	ADDU	src,src,64
	ADDU	dest,dest,64
	SUBU	rem,rem,64
2:
	BNZ		rem,3b							# stop if rem == 0
	# now copy the remaining words
	SET		rem,len							# the remaining bytes
	ANDNL	rem,#0007						# word-align it
	SUBU	len,len,rem						# the remaining bytes after this loop
	JMP		2f
3:
	LDOU	tmp,src,off
	STOU	tmp,dest,off
	ADDU	off,off,8
	SUBU	rem,rem,8
2:
	BNZ		rem,3b							# stop if rem == 0
	# maybe, there are some bytes left to copy
	SET		rem,len
	JMP		1f
	# default version: byte copy
2:
	LDBU	tmp,src,off
	STBU	tmp,dest,off
	ADDU	off,off,1
	SUBU	rem,rem,1
1:
	BNZ		rem,2b							# stop if rem == 0
3:
	POP		1,0
