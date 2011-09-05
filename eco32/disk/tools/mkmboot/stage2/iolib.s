;
; iolib.s -- a very small library of I/O routines
;

	.set	cin,0xE0000010
	.set	cout,0xE0000018
	.set	dskio,0xE0000030

	.export	getc
	.export	putc
	.export	rwscts

	.code
	.align	4

getc:
	add	$8,$0,cin
	jr	$8

putc:
	add	$8,$0,cout
	jr	$8

rwscts:
	add	$8,$0,dskio
	jr	$8
