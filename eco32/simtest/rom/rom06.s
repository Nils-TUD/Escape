;
; rom06.s -- "crossed" echo with two terminals, polled
;

	.set	tba,0xF0300000	; terminal base address

	add	$8,$0,tba	; set terminal base address
L1:
	ldw	$9,$8,0		; load receiver status into $9
	and	$9,$9,1		; check receiver ready
	beq	$9,$0,L3	; not ready - check other terminal
	ldw	$10,$8,4	; load receiver data into $10
L2:
	ldw	$9,$8,24	; load transmitter status into $9
	and	$9,$9,1		; check transmitter ready
	beq	$9,$0,L2	; loop while not ready
	stw	$10,$8,28	; load char into transmitter data
L3:
	ldw	$9,$8,16	; load receiver status into $9
	and	$9,$9,1		; check receiver ready
	beq	$9,$0,L1	; not ready - check other terminal
	ldw	$10,$8,20	; load receiver data into $10
L4:
	ldw	$9,$8,8		; load transmitter status into $9
	and	$9,$9,1		; check transmitter ready
	beq	$9,$0,L4	; loop while not ready
	stw	$10,$8,12	; load char into transmitter data
	j	L1		; all over again
