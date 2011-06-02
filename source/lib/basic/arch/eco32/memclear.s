/**
 * $Id$
 */

.global memclear

# void memclear(void *dest,size_t count)
memclear:
	.nosyn
	add		$10,$4,$5										# dest-end
	and		$8,$4,3
	beq		$8,$0,3f										# already word-aligned?
2:
	# ok, word-align dest
	add		$9,$0,4
	sub		$8,$9,$8										# remaining = 4 - (addr & 3)
	add		$9,$4,$8										# dest + number of bytes to copy before word-aligned
	bgeu	$5,$8,5f										# less than len?
	add		$9,$4,$5										# no, so take len as number of bytes
	j			5f
4:
	stb		$0,$4,0
	add		$4,$4,1
5:
	bltu	$4,$9,4b										# stop if $4 >= $9
3:
	# now it is word aligned
	# first, clear with loop-unrolling
	sub		$8,$10,$4										# $8 = number of remaining bytes
	ldhi	$11,0xFFFF0000							# save $12 for later usage
	or		$9,$11,0xFFE0
	and		$9,$8,$9										# align it to 8*4 bytes
	add		$9,$9,$4										# add dest
	j			2f
3:
	stw		$0,$4,0											# word 1
	stw		$0,$4,4											# word 2
	stw		$0,$4,8											# word 3
	stw		$0,$4,12										# word 4
	stw		$0,$4,16										# word 5
	stw		$0,$4,20										# word 6
	stw		$0,$4,24										# word 7
	stw		$0,$4,28										# word 8
	add		$4,$4,32
2:
	bltu	$4,$9,3b										# stop if $4 >= $9
	# now clear the remaining words
	sub		$8,$10,$4										# $8 = number of remaining bytes
	or		$9,$11,0xFFFC
	and		$9,$8,$9										# word-align it
	add		$9,$9,$4										# add dest
	j			2f
3:
	stw		$0,$4,0
	add		$4,$4,4
2:
	bltu	$4,$9,3b										# stop if $4 >= $9
	# maybe, there are some bytes left to copy
	j			1f
	# default version: byte copy
2:
	stb		$0,$4,0
	add		$4,$4,1
1:
	bltu	$4,$10,2b										# stop if dest-end reached
3:
	jr		$31
	.syn
