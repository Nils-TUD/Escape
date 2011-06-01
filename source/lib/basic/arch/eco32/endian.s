/**
 * $Id$
 */

.global le16tocpu
.global le32tocpu
.global cputole16
.global cputole32

cputole16:
le16tocpu:
	.nosyn
	and		$2,$4,0xFF00
	slr		$2,$2,8
	and		$8,$4,0xFF
	sll		$8,$8,8
	or		$2,$2,$8
	jr		$31
	.syn

cputole32:
le32tocpu:
	.nosyn
	and		$2,$4,0xFF00
	sll		$2,$2,8
	and		$8,$4,0xFF
	sll		$8,$8,24
	or		$2,$2,$8
	slr		$8,$4,8
	and		$8,$8,0xFF00
	or		$2,$2,$8
	slr		$8,$4,24
	or		$2,$2,$8
	jr		$31
	.syn
