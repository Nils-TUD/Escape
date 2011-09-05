%
% cset.mms -- tests conditional-set-instructions (generated)
%
		LOC		#1000

Main	NOR		$2,$2,$2	% = -1
		SET		$3,0
		SET		$4,1
		SET		$5,2
		SETH	$6,#8000

		% test CSN, #FFFFFFFFFFFFFFFF
		SET		$7,1
		CSN		$7,$2,$5

		% test CSN, #0
		SET		$8,1
		CSN		$8,$3,$5

		% test CSN, #1
		SET		$9,1
		CSN		$9,$4,$5

		% test CSN, #2
		SET		$10,1
		CSN		$10,$5,$5

		% test CSN, #8000000000000000
		SET		$11,1
		CSN		$11,$6,$5

		% test CSN, #FFFFFFFFFFFFFFFF
		SET		$12,1
		CSN		$12,$2,2

		% test CSN, #0
		SET		$13,1
		CSN		$13,$3,2

		% test CSN, #1
		SET		$14,1
		CSN		$14,$4,2

		% test CSN, #2
		SET		$15,1
		CSN		$15,$5,2

		% test CSN, #8000000000000000
		SET		$16,1
		CSN		$16,$6,2

		% test CSZ, #FFFFFFFFFFFFFFFF
		SET		$17,1
		CSZ		$17,$2,$5

		% test CSZ, #0
		SET		$18,1
		CSZ		$18,$3,$5

		% test CSZ, #1
		SET		$19,1
		CSZ		$19,$4,$5

		% test CSZ, #2
		SET		$20,1
		CSZ		$20,$5,$5

		% test CSZ, #8000000000000000
		SET		$21,1
		CSZ		$21,$6,$5

		% test CSZ, #FFFFFFFFFFFFFFFF
		SET		$22,1
		CSZ		$22,$2,2

		% test CSZ, #0
		SET		$23,1
		CSZ		$23,$3,2

		% test CSZ, #1
		SET		$24,1
		CSZ		$24,$4,2

		% test CSZ, #2
		SET		$25,1
		CSZ		$25,$5,2

		% test CSZ, #8000000000000000
		SET		$26,1
		CSZ		$26,$6,2

		% test CSP, #FFFFFFFFFFFFFFFF
		SET		$27,1
		CSP		$27,$2,$5

		% test CSP, #0
		SET		$28,1
		CSP		$28,$3,$5

		% test CSP, #1
		SET		$29,1
		CSP		$29,$4,$5

		% test CSP, #2
		SET		$30,1
		CSP		$30,$5,$5

		% test CSP, #8000000000000000
		SET		$31,1
		CSP		$31,$6,$5

		% test CSP, #FFFFFFFFFFFFFFFF
		SET		$32,1
		CSP		$32,$2,2

		% test CSP, #0
		SET		$33,1
		CSP		$33,$3,2

		% test CSP, #1
		SET		$34,1
		CSP		$34,$4,2

		% test CSP, #2
		SET		$35,1
		CSP		$35,$5,2

		% test CSP, #8000000000000000
		SET		$36,1
		CSP		$36,$6,2

		% test CSOD, #FFFFFFFFFFFFFFFF
		SET		$37,1
		CSOD		$37,$2,$5

		% test CSOD, #0
		SET		$38,1
		CSOD		$38,$3,$5

		% test CSOD, #1
		SET		$39,1
		CSOD		$39,$4,$5

		% test CSOD, #2
		SET		$40,1
		CSOD		$40,$5,$5

		% test CSOD, #8000000000000000
		SET		$41,1
		CSOD		$41,$6,$5

		% test CSOD, #FFFFFFFFFFFFFFFF
		SET		$42,1
		CSOD		$42,$2,2

		% test CSOD, #0
		SET		$43,1
		CSOD		$43,$3,2

		% test CSOD, #1
		SET		$44,1
		CSOD		$44,$4,2

		% test CSOD, #2
		SET		$45,1
		CSOD		$45,$5,2

		% test CSOD, #8000000000000000
		SET		$46,1
		CSOD		$46,$6,2

		% test CSNN, #FFFFFFFFFFFFFFFF
		SET		$47,1
		CSNN		$47,$2,$5

		% test CSNN, #0
		SET		$48,1
		CSNN		$48,$3,$5

		% test CSNN, #1
		SET		$49,1
		CSNN		$49,$4,$5

		% test CSNN, #2
		SET		$50,1
		CSNN		$50,$5,$5

		% test CSNN, #8000000000000000
		SET		$51,1
		CSNN		$51,$6,$5

		% test CSNN, #FFFFFFFFFFFFFFFF
		SET		$52,1
		CSNN		$52,$2,2

		% test CSNN, #0
		SET		$53,1
		CSNN		$53,$3,2

		% test CSNN, #1
		SET		$54,1
		CSNN		$54,$4,2

		% test CSNN, #2
		SET		$55,1
		CSNN		$55,$5,2

		% test CSNN, #8000000000000000
		SET		$56,1
		CSNN		$56,$6,2

		% test CSNZ, #FFFFFFFFFFFFFFFF
		SET		$57,1
		CSNZ		$57,$2,$5

		% test CSNZ, #0
		SET		$58,1
		CSNZ		$58,$3,$5

		% test CSNZ, #1
		SET		$59,1
		CSNZ		$59,$4,$5

		% test CSNZ, #2
		SET		$60,1
		CSNZ		$60,$5,$5

		% test CSNZ, #8000000000000000
		SET		$61,1
		CSNZ		$61,$6,$5

		% test CSNZ, #FFFFFFFFFFFFFFFF
		SET		$62,1
		CSNZ		$62,$2,2

		% test CSNZ, #0
		SET		$63,1
		CSNZ		$63,$3,2

		% test CSNZ, #1
		SET		$64,1
		CSNZ		$64,$4,2

		% test CSNZ, #2
		SET		$65,1
		CSNZ		$65,$5,2

		% test CSNZ, #8000000000000000
		SET		$66,1
		CSNZ		$66,$6,2

		% test CSNP, #FFFFFFFFFFFFFFFF
		SET		$67,1
		CSNP		$67,$2,$5

		% test CSNP, #0
		SET		$68,1
		CSNP		$68,$3,$5

		% test CSNP, #1
		SET		$69,1
		CSNP		$69,$4,$5

		% test CSNP, #2
		SET		$70,1
		CSNP		$70,$5,$5

		% test CSNP, #8000000000000000
		SET		$71,1
		CSNP		$71,$6,$5

		% test CSNP, #FFFFFFFFFFFFFFFF
		SET		$72,1
		CSNP		$72,$2,2

		% test CSNP, #0
		SET		$73,1
		CSNP		$73,$3,2

		% test CSNP, #1
		SET		$74,1
		CSNP		$74,$4,2

		% test CSNP, #2
		SET		$75,1
		CSNP		$75,$5,2

		% test CSNP, #8000000000000000
		SET		$76,1
		CSNP		$76,$6,2

		% test CSEV, #FFFFFFFFFFFFFFFF
		SET		$77,1
		CSEV		$77,$2,$5

		% test CSEV, #0
		SET		$78,1
		CSEV		$78,$3,$5

		% test CSEV, #1
		SET		$79,1
		CSEV		$79,$4,$5

		% test CSEV, #2
		SET		$80,1
		CSEV		$80,$5,$5

		% test CSEV, #8000000000000000
		SET		$81,1
		CSEV		$81,$6,$5

		% test CSEV, #FFFFFFFFFFFFFFFF
		SET		$82,1
		CSEV		$82,$2,2

		% test CSEV, #0
		SET		$83,1
		CSEV		$83,$3,2

		% test CSEV, #1
		SET		$84,1
		CSEV		$84,$4,2

		% test CSEV, #2
		SET		$85,1
		CSEV		$85,$5,2

		% test CSEV, #8000000000000000
		SET		$86,1
		CSEV		$86,$6,2

		TRAP	0
