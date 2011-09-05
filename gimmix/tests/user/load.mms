%
% load.mms -- tests load-instructions (generated)
%

TEST1	BYTE	#FF,#FE,1,2
		LOC		@ + (2 - @) & 1		% align to 2
TEST2	WYDE	#FFFF,#FFFE,1,2
		LOC		@ + (4 - @) & 3		% align to 4
TEST3	TETRA	#FFFFFFFF,#FFFFFFFE,1,2
		LOC		@ + (8 - @) & 7		% align to 8
TEST4	OCTA	#FFFFFFFFFFFFFFFF,#FFFFFFFFFFFFFFFE,1,2

		LOC		#1000

Main	SET		$2,TEST1
		SET		$3,TEST2
		SET		$4,TEST3
		SET		$5,TEST4

		% test LDB(I)
		LDB		$6,$2,0
		SET		$7,0
		LDB		$7,$2,$7
		LDB		$8,$2,1
		SET		$9,1
		LDB		$9,$2,$9
		LDB		$10,$2,2
		SET		$11,2
		LDB		$11,$2,$11
		LDB		$12,$2,3
		SET		$13,3
		LDB		$13,$2,$13

		% test LDBU(I)
		LDBU		$14,$2,0
		SET		$15,0
		LDBU		$15,$2,$15
		LDBU		$16,$2,1
		SET		$17,1
		LDBU		$17,$2,$17
		LDBU		$18,$2,2
		SET		$19,2
		LDBU		$19,$2,$19
		LDBU		$20,$2,3
		SET		$21,3
		LDBU		$21,$2,$21

		% test LDW(I)
		LDW		$22,$3,0
		SET		$23,0
		LDW		$23,$3,$23
		LDW		$24,$3,2
		SET		$25,2
		LDW		$25,$3,$25
		LDW		$26,$3,4
		SET		$27,4
		LDW		$27,$3,$27
		LDW		$28,$3,6
		SET		$29,6
		LDW		$29,$3,$29

		% test LDWU(I)
		LDWU		$30,$3,0
		SET		$31,0
		LDWU		$31,$3,$31
		LDWU		$32,$3,2
		SET		$33,2
		LDWU		$33,$3,$33
		LDWU		$34,$3,4
		SET		$35,4
		LDWU		$35,$3,$35
		LDWU		$36,$3,6
		SET		$37,6
		LDWU		$37,$3,$37

		% test LDT(I)
		LDT		$38,$4,0
		SET		$39,0
		LDT		$39,$4,$39
		LDT		$40,$4,4
		SET		$41,4
		LDT		$41,$4,$41
		LDT		$42,$4,8
		SET		$43,8
		LDT		$43,$4,$43
		LDT		$44,$4,12
		SET		$45,12
		LDT		$45,$4,$45

		% test LDTU(I)
		LDTU		$46,$4,0
		SET		$47,0
		LDTU		$47,$4,$47
		LDTU		$48,$4,4
		SET		$49,4
		LDTU		$49,$4,$49
		LDTU		$50,$4,8
		SET		$51,8
		LDTU		$51,$4,$51
		LDTU		$52,$4,12
		SET		$53,12
		LDTU		$53,$4,$53

		% test LDO(I)
		LDO		$54,$5,0
		SET		$55,0
		LDO		$55,$5,$55
		LDO		$56,$5,8
		SET		$57,8
		LDO		$57,$5,$57
		LDO		$58,$5,16
		SET		$59,16
		LDO		$59,$5,$59
		LDO		$60,$5,24
		SET		$61,24
		LDO		$61,$5,$61

		% test LDOU(I)
		LDOU		$62,$5,0
		SET		$63,0
		LDOU		$63,$5,$63
		LDOU		$64,$5,8
		SET		$65,8
		LDOU		$65,$5,$65
		LDOU		$66,$5,16
		SET		$67,16
		LDOU		$67,$5,$67
		LDOU		$68,$5,24
		SET		$69,24
		LDOU		$69,$5,$69

		% test LDHT(I)
		LDHT		$70,$4,0
		SET		$71,0
		LDHT		$71,$4,$71
		LDHT		$72,$4,4
		SET		$73,4
		LDHT		$73,$4,$73
		LDHT		$74,$4,8
		SET		$75,8
		LDHT		$75,$4,$75
		LDHT		$76,$4,12
		SET		$77,12
		LDHT		$77,$4,$77

		TRAP	0
