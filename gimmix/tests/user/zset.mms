%
% zset.mms -- tests zero-or-set-instructions (generated)
%
		LOC		#1000

Main	NOR		$2,$2,$2	% = -1
		SET		$3,0
		SET		$4,1
		SET		$5,2
		SETH	$6,#8000

		% test ZSN, #FFFFFFFFFFFFFFFF
		SET		$7,1
		ZSN		$7,$2,$5

		% test ZSN, #0
		SET		$8,1
		ZSN		$8,$3,$5

		% test ZSN, #1
		SET		$9,1
		ZSN		$9,$4,$5

		% test ZSN, #2
		SET		$10,1
		ZSN		$10,$5,$5

		% test ZSN, #8000000000000000
		SET		$11,1
		ZSN		$11,$6,$5

		% test ZSN, #FFFFFFFFFFFFFFFF
		SET		$12,1
		ZSN		$12,$2,2

		% test ZSN, #0
		SET		$13,1
		ZSN		$13,$3,2

		% test ZSN, #1
		SET		$14,1
		ZSN		$14,$4,2

		% test ZSN, #2
		SET		$15,1
		ZSN		$15,$5,2

		% test ZSN, #8000000000000000
		SET		$16,1
		ZSN		$16,$6,2

		% test ZSZ, #FFFFFFFFFFFFFFFF
		SET		$17,1
		ZSZ		$17,$2,$5

		% test ZSZ, #0
		SET		$18,1
		ZSZ		$18,$3,$5

		% test ZSZ, #1
		SET		$19,1
		ZSZ		$19,$4,$5

		% test ZSZ, #2
		SET		$20,1
		ZSZ		$20,$5,$5

		% test ZSZ, #8000000000000000
		SET		$21,1
		ZSZ		$21,$6,$5

		% test ZSZ, #FFFFFFFFFFFFFFFF
		SET		$22,1
		ZSZ		$22,$2,2

		% test ZSZ, #0
		SET		$23,1
		ZSZ		$23,$3,2

		% test ZSZ, #1
		SET		$24,1
		ZSZ		$24,$4,2

		% test ZSZ, #2
		SET		$25,1
		ZSZ		$25,$5,2

		% test ZSZ, #8000000000000000
		SET		$26,1
		ZSZ		$26,$6,2

		% test ZSP, #FFFFFFFFFFFFFFFF
		SET		$27,1
		ZSP		$27,$2,$5

		% test ZSP, #0
		SET		$28,1
		ZSP		$28,$3,$5

		% test ZSP, #1
		SET		$29,1
		ZSP		$29,$4,$5

		% test ZSP, #2
		SET		$30,1
		ZSP		$30,$5,$5

		% test ZSP, #8000000000000000
		SET		$31,1
		ZSP		$31,$6,$5

		% test ZSP, #FFFFFFFFFFFFFFFF
		SET		$32,1
		ZSP		$32,$2,2

		% test ZSP, #0
		SET		$33,1
		ZSP		$33,$3,2

		% test ZSP, #1
		SET		$34,1
		ZSP		$34,$4,2

		% test ZSP, #2
		SET		$35,1
		ZSP		$35,$5,2

		% test ZSP, #8000000000000000
		SET		$36,1
		ZSP		$36,$6,2

		% test ZSOD, #FFFFFFFFFFFFFFFF
		SET		$37,1
		ZSOD		$37,$2,$5

		% test ZSOD, #0
		SET		$38,1
		ZSOD		$38,$3,$5

		% test ZSOD, #1
		SET		$39,1
		ZSOD		$39,$4,$5

		% test ZSOD, #2
		SET		$40,1
		ZSOD		$40,$5,$5

		% test ZSOD, #8000000000000000
		SET		$41,1
		ZSOD		$41,$6,$5

		% test ZSOD, #FFFFFFFFFFFFFFFF
		SET		$42,1
		ZSOD		$42,$2,2

		% test ZSOD, #0
		SET		$43,1
		ZSOD		$43,$3,2

		% test ZSOD, #1
		SET		$44,1
		ZSOD		$44,$4,2

		% test ZSOD, #2
		SET		$45,1
		ZSOD		$45,$5,2

		% test ZSOD, #8000000000000000
		SET		$46,1
		ZSOD		$46,$6,2

		% test ZSNN, #FFFFFFFFFFFFFFFF
		SET		$47,1
		ZSNN		$47,$2,$5

		% test ZSNN, #0
		SET		$48,1
		ZSNN		$48,$3,$5

		% test ZSNN, #1
		SET		$49,1
		ZSNN		$49,$4,$5

		% test ZSNN, #2
		SET		$50,1
		ZSNN		$50,$5,$5

		% test ZSNN, #8000000000000000
		SET		$51,1
		ZSNN		$51,$6,$5

		% test ZSNN, #FFFFFFFFFFFFFFFF
		SET		$52,1
		ZSNN		$52,$2,2

		% test ZSNN, #0
		SET		$53,1
		ZSNN		$53,$3,2

		% test ZSNN, #1
		SET		$54,1
		ZSNN		$54,$4,2

		% test ZSNN, #2
		SET		$55,1
		ZSNN		$55,$5,2

		% test ZSNN, #8000000000000000
		SET		$56,1
		ZSNN		$56,$6,2

		% test ZSNZ, #FFFFFFFFFFFFFFFF
		SET		$57,1
		ZSNZ		$57,$2,$5

		% test ZSNZ, #0
		SET		$58,1
		ZSNZ		$58,$3,$5

		% test ZSNZ, #1
		SET		$59,1
		ZSNZ		$59,$4,$5

		% test ZSNZ, #2
		SET		$60,1
		ZSNZ		$60,$5,$5

		% test ZSNZ, #8000000000000000
		SET		$61,1
		ZSNZ		$61,$6,$5

		% test ZSNZ, #FFFFFFFFFFFFFFFF
		SET		$62,1
		ZSNZ		$62,$2,2

		% test ZSNZ, #0
		SET		$63,1
		ZSNZ		$63,$3,2

		% test ZSNZ, #1
		SET		$64,1
		ZSNZ		$64,$4,2

		% test ZSNZ, #2
		SET		$65,1
		ZSNZ		$65,$5,2

		% test ZSNZ, #8000000000000000
		SET		$66,1
		ZSNZ		$66,$6,2

		% test ZSNP, #FFFFFFFFFFFFFFFF
		SET		$67,1
		ZSNP		$67,$2,$5

		% test ZSNP, #0
		SET		$68,1
		ZSNP		$68,$3,$5

		% test ZSNP, #1
		SET		$69,1
		ZSNP		$69,$4,$5

		% test ZSNP, #2
		SET		$70,1
		ZSNP		$70,$5,$5

		% test ZSNP, #8000000000000000
		SET		$71,1
		ZSNP		$71,$6,$5

		% test ZSNP, #FFFFFFFFFFFFFFFF
		SET		$72,1
		ZSNP		$72,$2,2

		% test ZSNP, #0
		SET		$73,1
		ZSNP		$73,$3,2

		% test ZSNP, #1
		SET		$74,1
		ZSNP		$74,$4,2

		% test ZSNP, #2
		SET		$75,1
		ZSNP		$75,$5,2

		% test ZSNP, #8000000000000000
		SET		$76,1
		ZSNP		$76,$6,2

		% test ZSEV, #FFFFFFFFFFFFFFFF
		SET		$77,1
		ZSEV		$77,$2,$5

		% test ZSEV, #0
		SET		$78,1
		ZSEV		$78,$3,$5

		% test ZSEV, #1
		SET		$79,1
		ZSEV		$79,$4,$5

		% test ZSEV, #2
		SET		$80,1
		ZSEV		$80,$5,$5

		% test ZSEV, #8000000000000000
		SET		$81,1
		ZSEV		$81,$6,$5

		% test ZSEV, #FFFFFFFFFFFFFFFF
		SET		$82,1
		ZSEV		$82,$2,2

		% test ZSEV, #0
		SET		$83,1
		ZSEV		$83,$3,2

		% test ZSEV, #1
		SET		$84,1
		ZSEV		$84,$4,2

		% test ZSEV, #2
		SET		$85,1
		ZSEV		$85,$5,2

		% test ZSEV, #8000000000000000
		SET		$86,1
		ZSEV		$86,$6,2

		TRAP	0
