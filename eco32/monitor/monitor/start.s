;
; start.s -- ECO32 ROM monitor startup and support routines
;

	.set	dmapaddr,0xC0000000	; base of directly mapped addresses
	.set	stacktop,0xC0400000	; monitor stack is at top of memory

	.set	PSW,0			; reg # of PSW
	.set	TLB_INDEX,1		; reg # of TLB Index
	.set	TLB_ENTRY_HI,2		; reg # of TLB EntryHi
	.set	TLB_ENTRY_LO,3		; reg # of TLB EntryLo
	.set	TLB_ENTRIES,32		; number of TLB entries

	.set	USER_CONTEXT_SIZE,36*4	; size of user context

;***************************************************************

	.import	_ecode
	.import	_edata
	.import	_ebss

	.import	kbdinit
	.import	kbdinchk
	.import	kbdin

	.import	dspinit
	.import	dspoutchk
	.import	dspout

	.import	serinit
	.import	ser0inchk
	.import	ser0in
	.import	ser0outchk
	.import	ser0out

	.import	sctcapctl
	.import	sctioctl
	.import	sctcapser
	.import	sctioser

	.import	main

	.export	_bcode
	.export	_bdata
	.export	_bbss

	.export	cinchk
	.export	cin
	.export	coutchk
	.export	cout
	.export	sinchk
	.export	sin
	.export	soutchk
	.export	sout
	.export	dskcap
	.export	dskio

	.export	getTLB_HI
	.export	getTLB_LO
	.export	setTLB

	.export	saveState
	.export	monitorReturn

	.import	userContext
	.export	resume

;***************************************************************

	.code
_bcode:

	.data
_bdata:

	.bss
_bbss:

;***************************************************************

	.code
	.align	4

reset:
	j	start

interrupt:
	j	isr

userMiss:
	j	umsr

;***************************************************************

	.code
	.align	4

cinchk:
;	j	kbdinchk
	j	ser0inchk

cin:
;	j	kbdin
	j	ser0in

coutchk:
;	j	dspoutchk
	j	ser0outchk

cout:
;	j	dspout
	j	ser0out

sinchk:
	j	ser0inchk

sin:
	j	ser0in

soutchk:
	j	ser0outchk

sout:
	j	ser0out

dskcap:
	j	dcap

dskio:
	j	dio

;***************************************************************

	.code
	.align	4

start:
	; force CPU into a defined state
	mvts	$0,PSW			; disable interrupts and user mode

	; initialize TLB
	mvts	$0,TLB_ENTRY_LO		; invalidate all TLB entries
	add	$8,$0,dmapaddr		; by impossible virtual page number
	add	$9,$0,$0
	add	$10,$0,TLB_ENTRIES
tlbloop:
	mvts	$8,TLB_ENTRY_HI
	mvts	$9,TLB_INDEX
	tbwi
	add	$8,$8,0x1000		; all entries must be different
	add	$9,$9,1
	bne	$9,$10,tlbloop

	; copy data segment
	add	$10,$0,_bdata		; lowest dst addr to be written to
	add	$8,$0,_edata		; one above the top dst addr
	sub	$9,$8,$10		; $9 = size of data segment
	add	$9,$9,_ecode		; data is waiting right after code
	j	cpytest
cpyloop:
	ldw	$11,$9,0		; src addr in $9
	stw	$11,$8,0		; dst addr in $8
cpytest:
	sub	$8,$8,4			; downward
	sub	$9,$9,4
	bgeu	$8,$10,cpyloop

	; clear bss segment
	add	$8,$0,_bbss		; start with first word of bss
	add	$9,$0,_ebss		; this is one above the top
	j	clrtest
clrloop:
	stw	$0,$8,0			; dst addr in $8
	add	$8,$8,4			; upward
clrtest:
	bltu	$8,$9,clrloop

	; now do some useful work
	add	$29,$0,stacktop		; setup monitor stack
;	jal	dspinit			; init display
;	jal	kbdinit			; init keyboard
	jal	serinit			; init serial interface
	jal	main			; enter command loop

	; main should never return
	j	start			; just to be sure...

;***************************************************************

	; Word getTLB_HI(int index)
getTLB_HI:
	mvts	$4,TLB_INDEX
	tbri
	mvfs	$2,TLB_ENTRY_HI
	jr	$31

	; Word getTLB_LO(int index)
getTLB_LO:
	mvts	$4,TLB_INDEX
	tbri
	mvfs	$2,TLB_ENTRY_LO
	jr	$31

	; void setTLB(int index, Word entryHi, Word entryLo)
setTLB:
	mvts	$4,TLB_INDEX
	mvts	$5,TLB_ENTRY_HI
	mvts	$6,TLB_ENTRY_LO
	tbwi
	jr	$31

;***************************************************************

	; int dskcap(int dskno)
dcap:
	bne	$4,$0,dcapser
	j	sctcapctl
dcapser:
	j	sctcapser

	; int dskio(int dskno, char cmd, int sct, Word addr, int nscts)
dio:
	bne	$4,$0,dioser
	add	$4,$5,$0
	add	$5,$6,$0
	add	$6,$7,$0
	ldw	$7,$29,16
	j	sctioctl
dioser:
	add	$4,$5,$0
	add	$5,$6,$0
	add	$6,$7,$0
	ldw	$7,$29,16
	j	sctioser

;***************************************************************

	.code
	.align	4

	; Bool saveState(MonitorState *msp)
	; always return 'true' here
saveState:
	stw	$31,$4,0*4		; return address
	stw	$29,$4,1*4		; stack pointer
	stw	$16,$4,2*4		; local variables
	stw	$17,$4,3*4
	stw	$18,$4,4*4
	stw	$19,$4,5*4
	stw	$20,$4,6*4
	stw	$21,$4,7*4
	stw	$22,$4,8*4
	stw	$23,$4,9*4
	add	$2,$0,1
	jr	$31

	; load state when re-entering monitor
	; this appears as if returning from saveState
	; but the return value is 'false' here
loadState:
	ldw	$8,$0,monitorReturn
	beq	$8,$0,loadState		; fatal error: monitor state lost
	ldw	$31,$8,0*4		; return address
	ldw	$29,$8,1*4		; stack pointer
	ldw	$16,$8,2*4		; local variables
	ldw	$17,$8,3*4
	ldw	$18,$8,4*4
	ldw	$19,$8,5*4
	ldw	$20,$8,6*4
	ldw	$21,$8,7*4
	ldw	$22,$8,8*4
	ldw	$23,$8,9*4
	add	$2,$0,0
	jr	$31

	.bss
	.align	4

	; extern MonitorState *monitorReturn
monitorReturn:
	.space	4

	; extern UserContext userContext
userContext:
	.space	USER_CONTEXT_SIZE

;***************************************************************

	.code
	.align	4

	; void resume(void)
	; use userContext to load state
resume:
	mvts	$0,PSW
	add	$28,$0,userContext
	.nosyn
	ldw	$8,$28,33*4		; tlbIndex
	mvts	$8,TLB_INDEX
	ldw	$8,$28,34*4		; tlbWntryHi
	mvts	$8,TLB_ENTRY_HI
	ldw	$8,$28,35*4		; tlbEntryLo
	mvts	$8,TLB_ENTRY_LO
	;ldw	$0,$28,0*4		; registers
	ldw	$1,$28,1*4
	ldw	$2,$28,2*4
	ldw	$3,$28,3*4
	ldw	$4,$28,4*4
	ldw	$5,$28,5*4
	ldw	$6,$28,6*4
	ldw	$7,$28,7*4
	ldw	$8,$28,8*4
	ldw	$9,$28,9*4
	ldw	$10,$28,10*4
	ldw	$11,$28,11*4
	ldw	$12,$28,12*4
	ldw	$13,$28,13*4
	ldw	$14,$28,14*4
	ldw	$15,$28,15*4
	ldw	$16,$28,16*4
	ldw	$17,$28,17*4
	ldw	$18,$28,18*4
	ldw	$19,$28,19*4
	ldw	$20,$28,20*4
	ldw	$21,$28,21*4
	ldw	$22,$28,22*4
	ldw	$23,$28,23*4
	ldw	$24,$28,24*4
	ldw	$25,$28,25*4
	ldw	$26,$28,26*4
	ldw	$27,$28,27*4
	;ldw	$28,$28,28*4
	ldw	$29,$28,29*4
	ldw	$30,$28,30*4
	ldw	$31,$28,31*4
	ldw	$28,$28,32*4		; psw
	mvts	$28,PSW
	rfx
	.syn

	; interrupt entry
	; use userContext to store state
isr:
umsr:
	.nosyn
	ldhi	$28,userContext
	or	$28,$28,userContext
	stw	$0,$28,0*4		; registers
	stw	$1,$28,1*4
	stw	$2,$28,2*4
	stw	$3,$28,3*4
	stw	$4,$28,4*4
	stw	$5,$28,5*4
	stw	$6,$28,6*4
	stw	$7,$28,7*4
	stw	$8,$28,8*4
	stw	$9,$28,9*4
	stw	$10,$28,10*4
	stw	$11,$28,11*4
	stw	$12,$28,12*4
	stw	$13,$28,13*4
	stw	$14,$28,14*4
	stw	$15,$28,15*4
	stw	$16,$28,16*4
	stw	$17,$28,17*4
	stw	$18,$28,18*4
	stw	$19,$28,19*4
	stw	$20,$28,20*4
	stw	$21,$28,21*4
	stw	$22,$28,22*4
	stw	$23,$28,23*4
	stw	$24,$28,24*4
	stw	$25,$28,25*4
	stw	$26,$28,26*4
	stw	$27,$28,27*4
	stw	$28,$28,28*4
	stw	$29,$28,29*4
	stw	$30,$28,30*4
	stw	$31,$28,31*4
	mvfs	$8,PSW
	stw	$8,$28,32*4		; psw
	mvfs	$8,TLB_INDEX
	stw	$8,$28,33*4		; tlbIndex
	mvfs	$8,TLB_ENTRY_HI
	stw	$8,$28,34*4		; tlbEntryHi
	mvfs	$8,TLB_ENTRY_LO
	stw	$8,$28,35*4		; tlbEntryLo
	.syn
	j	loadState
