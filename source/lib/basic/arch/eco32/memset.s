/**
 * $Id$
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
	ldhi	$12,0xFFFF									# save $12 for later usage
	or		$9,$12,0xFFE0
	and		$9,$10,$9										# align $9 to 8 words
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
	add		$11,$4,32
2:
	bltu	$4,$9,3b										# stop if $4 >= $9
	# now clear the remaining words
	or		$9,$12,0xFFFC
	and		$9,$10,$9										# word align dest-end
	j			2f
3:
	stw		$11,$4,0
	add		$4,$4,4
2:
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
