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

.global memset

# void memset(void *addr,int value,size_t count)
memset:
	.nosyn
	add		$10,$4,$6										# dest-end
	and		$8,$4,3
	beq		$8,$0,3f										# already word-aligned?
2:
	# ok, word-align dest
	add		$9,$0,4
	sub		$8,$9,$8										# remaining = 4 - (addr & 3)
	add		$9,$4,$8										# dest + number of bytes to copy before word-aligned
	bgeu	$6,$8,5f										# less than len?
	add		$9,$4,$6										# no, so take len as number of bytes
	j			5f
4:
	stb		$5,$4,0
	add		$4,$4,1
5:
	bltu	$4,$9,4b										# stop if $4 >= $9
3:
	# now it is word aligned
	add		$11,$5,$0										# build word with 4 copies of the byte to store
	sll		$8,$5,8
	or		$11,$11,$8
	sll		$8,$5,16
	or		$11,$11,$8
	sll		$8,$5,24
	or		$11,$11,$8
	# first, copy with loop-unrolling
	sub		$8,$10,$4										# $8 = number of remaining bytes
	ldhi	$12,0xFFFF0000							# save $12 for later usage
	or		$9,$12,0xFFE0
	and		$9,$8,$9										# align it to 8*4 bytes
	add		$9,$9,$4										# add dest
	j			2f
3:
	stw		$11,$4,0										# word 1
	stw		$11,$4,4										# word 2
	stw		$11,$4,8										# word 3
	stw		$11,$4,12										# word 4
	stw		$11,$4,16										# word 5
	stw		$11,$4,20										# word 6
	stw		$11,$4,24										# word 7
	stw		$11,$4,28										# word 8
	add		$4,$4,32
2:
	bltu	$4,$9,3b										# stop if $4 >= $9
	# now clear the remaining words
	sub		$8,$10,$4										# $8 = number of remaining bytes
	or		$9,$12,0xFFFC
	and		$9,$8,$9										# word-align it
	add		$9,$9,$4										# add dest
	j			6f
3:
	stw		$11,$4,0
	add		$4,$4,4
6:
	bltu	$4,$9,3b										# stop if $4 >= $9
	# maybe, there are some bytes left to copy
	j			1f
	# default version: byte copy
2:
	stb		$5,$4,0
	add		$4,$4,1
1:
	bltu	$4,$10,2b										# stop if dest-end reached
3:
	jr		$31
	.syn
