/**
 * $Id$
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

# void *memcpy(void *dest,const void *src,size_t len)
memcpy:
	.nosyn
	add		$10,$4,$6										# dest-end
	and		$8,$4,3
	and		$9,$5,3
	bne		$8,$9,1f										# not the same alignment? then a byte-copy is required
	beq		$8,$0,3f										# already word-aligned?
2:
	# ok, word-align them
	add		$9,$0,4
	sub		$8,$9,$8										# remaining = 4 - (addr & 3)
	add		$9,$4,$8										# dest + number of bytes to copy before word-aligned
	bgeu	$6,$8,5f										# less than len?
	add		$9,$4,$6										# no, so take len as number of bytes
	j			5f
4:
	ldb		$8,$5,0
	stb		$8,$4,0
	add		$5,$5,1
	add		$4,$4,1
5:
	bltu	$4,$9,4b										# stop if $4 >= $9
3:
	# now they are word aligned
	# first, copy with loop-unrolling
	sub		$8,$10,$4										# $8 = number of remaining bytes
	ldhi	$11,0xFFFF0000							# save $11 for later usage
	or		$9,$11,0xFFE0
	and		$9,$8,$9										# align it to 8*4 bytes
	add		$9,$9,$4										# add dest
	j			2f
3:
	ldw		$8,$5,0
	stw		$8,$4,0											# word 1
	ldw		$8,$5,4
	stw		$8,$4,4											# word 2
	ldw		$8,$5,8
	stw		$8,$4,8											# word 3
	ldw		$8,$5,12
	stw		$8,$4,12										# word 4
	ldw		$8,$5,16
	stw		$8,$4,16										# word 5
	ldw		$8,$5,20
	stw		$8,$4,20										# word 6
	ldw		$8,$5,24
	stw		$8,$4,24										# word 7
	ldw		$8,$5,28
	stw		$8,$4,28										# word 8
	add		$5,$5,32
	add		$4,$4,32
2:
	bltu	$4,$9,3b										# stop if $4 >= $9
	# now copy the remaining words
	sub		$8,$10,$4										# $8 = number of remaining bytes
	or		$9,$11,0xFFFC
	and		$9,$8,$9										# word-align it
	add		$9,$9,$4										# add dest
	j			2f
3:
	ldw		$8,$5,0
	stw		$8,$4,0
	add		$5,$5,4
	add		$4,$4,4
2:
	bltu	$4,$9,3b										# stop if $4 >= $9
	# maybe, there are some bytes left to copy
	j			1f
	# default version: byte copy
2:
	ldb		$8,$5,0
	stb		$8,$4,0
	add		$5,$5,1
	add		$4,$4,1
1:
	bltu	$4,$10,2b										# stop if dest-end reached
3:
	sub		$2,$10,$6										# return dest
	jr		$31
	.syn
