%
% (p)branch.mms -- tests branch-instructions (generated)
%
		LOC		#1000

Main	NOR		$2,$2,$2	% = -1
		SET		$3,0
		SET		$4,1
		SET		$5,2
		SETH	$6,#8000

		% test BN, #FFFFFFFFFFFFFFFF
		BN		$2,1F
		SET		$7,1
		JMP		2F
1H		SET		$7,2
2H		OR		$7,$7,$7

		% test BN, #0
		BN		$3,1F
		SET		$8,1
		JMP		2F
1H		SET		$8,2
2H		OR		$8,$8,$8

		% test BN, #1
		BN		$4,1F
		SET		$9,1
		JMP		2F
1H		SET		$9,2
2H		OR		$9,$9,$9

		% test BN, #2
		BN		$5,1F
		SET		$10,1
		JMP		2F
1H		SET		$10,2
2H		OR		$10,$10,$10

		% test BN, #8000000000000000
		BN		$6,1F
		SET		$11,1
		JMP		2F
1H		SET		$11,2
2H		OR		$11,$11,$11

		% test BNB, #FFFFFFFFFFFFFFFF
		JMP		2F
1H		SET		$12,2
		JMP		3F
2H		BN		$2,1B
		SET		$12,1
3H		OR		$12,$12,$12

		% test BNB, #0
		JMP		2F
1H		SET		$13,2
		JMP		3F
2H		BN		$3,1B
		SET		$13,1
3H		OR		$13,$13,$13

		% test BNB, #1
		JMP		2F
1H		SET		$14,2
		JMP		3F
2H		BN		$4,1B
		SET		$14,1
3H		OR		$14,$14,$14

		% test BNB, #2
		JMP		2F
1H		SET		$15,2
		JMP		3F
2H		BN		$5,1B
		SET		$15,1
3H		OR		$15,$15,$15

		% test BNB, #8000000000000000
		JMP		2F
1H		SET		$16,2
		JMP		3F
2H		BN		$6,1B
		SET		$16,1
3H		OR		$16,$16,$16

		% test BZ, #FFFFFFFFFFFFFFFF
		BZ		$2,1F
		SET		$17,1
		JMP		2F
1H		SET		$17,2
2H		OR		$17,$17,$17

		% test BZ, #0
		BZ		$3,1F
		SET		$18,1
		JMP		2F
1H		SET		$18,2
2H		OR		$18,$18,$18

		% test BZ, #1
		BZ		$4,1F
		SET		$19,1
		JMP		2F
1H		SET		$19,2
2H		OR		$19,$19,$19

		% test BZ, #2
		BZ		$5,1F
		SET		$20,1
		JMP		2F
1H		SET		$20,2
2H		OR		$20,$20,$20

		% test BZ, #8000000000000000
		BZ		$6,1F
		SET		$21,1
		JMP		2F
1H		SET		$21,2
2H		OR		$21,$21,$21

		% test BZB, #FFFFFFFFFFFFFFFF
		JMP		2F
1H		SET		$22,2
		JMP		3F
2H		BZ		$2,1B
		SET		$22,1
3H		OR		$22,$22,$22

		% test BZB, #0
		JMP		2F
1H		SET		$23,2
		JMP		3F
2H		BZ		$3,1B
		SET		$23,1
3H		OR		$23,$23,$23

		% test BZB, #1
		JMP		2F
1H		SET		$24,2
		JMP		3F
2H		BZ		$4,1B
		SET		$24,1
3H		OR		$24,$24,$24

		% test BZB, #2
		JMP		2F
1H		SET		$25,2
		JMP		3F
2H		BZ		$5,1B
		SET		$25,1
3H		OR		$25,$25,$25

		% test BZB, #8000000000000000
		JMP		2F
1H		SET		$26,2
		JMP		3F
2H		BZ		$6,1B
		SET		$26,1
3H		OR		$26,$26,$26

		% test BP, #FFFFFFFFFFFFFFFF
		BP		$2,1F
		SET		$27,1
		JMP		2F
1H		SET		$27,2
2H		OR		$27,$27,$27

		% test BP, #0
		BP		$3,1F
		SET		$28,1
		JMP		2F
1H		SET		$28,2
2H		OR		$28,$28,$28

		% test BP, #1
		BP		$4,1F
		SET		$29,1
		JMP		2F
1H		SET		$29,2
2H		OR		$29,$29,$29

		% test BP, #2
		BP		$5,1F
		SET		$30,1
		JMP		2F
1H		SET		$30,2
2H		OR		$30,$30,$30

		% test BP, #8000000000000000
		BP		$6,1F
		SET		$31,1
		JMP		2F
1H		SET		$31,2
2H		OR		$31,$31,$31

		% test BPB, #FFFFFFFFFFFFFFFF
		JMP		2F
1H		SET		$32,2
		JMP		3F
2H		BP		$2,1B
		SET		$32,1
3H		OR		$32,$32,$32

		% test BPB, #0
		JMP		2F
1H		SET		$33,2
		JMP		3F
2H		BP		$3,1B
		SET		$33,1
3H		OR		$33,$33,$33

		% test BPB, #1
		JMP		2F
1H		SET		$34,2
		JMP		3F
2H		BP		$4,1B
		SET		$34,1
3H		OR		$34,$34,$34

		% test BPB, #2
		JMP		2F
1H		SET		$35,2
		JMP		3F
2H		BP		$5,1B
		SET		$35,1
3H		OR		$35,$35,$35

		% test BPB, #8000000000000000
		JMP		2F
1H		SET		$36,2
		JMP		3F
2H		BP		$6,1B
		SET		$36,1
3H		OR		$36,$36,$36

		% test BOD, #FFFFFFFFFFFFFFFF
		BOD		$2,1F
		SET		$37,1
		JMP		2F
1H		SET		$37,2
2H		OR		$37,$37,$37

		% test BOD, #0
		BOD		$3,1F
		SET		$38,1
		JMP		2F
1H		SET		$38,2
2H		OR		$38,$38,$38

		% test BOD, #1
		BOD		$4,1F
		SET		$39,1
		JMP		2F
1H		SET		$39,2
2H		OR		$39,$39,$39

		% test BOD, #2
		BOD		$5,1F
		SET		$40,1
		JMP		2F
1H		SET		$40,2
2H		OR		$40,$40,$40

		% test BOD, #8000000000000000
		BOD		$6,1F
		SET		$41,1
		JMP		2F
1H		SET		$41,2
2H		OR		$41,$41,$41

		% test BODB, #FFFFFFFFFFFFFFFF
		JMP		2F
1H		SET		$42,2
		JMP		3F
2H		BOD		$2,1B
		SET		$42,1
3H		OR		$42,$42,$42

		% test BODB, #0
		JMP		2F
1H		SET		$43,2
		JMP		3F
2H		BOD		$3,1B
		SET		$43,1
3H		OR		$43,$43,$43

		% test BODB, #1
		JMP		2F
1H		SET		$44,2
		JMP		3F
2H		BOD		$4,1B
		SET		$44,1
3H		OR		$44,$44,$44

		% test BODB, #2
		JMP		2F
1H		SET		$45,2
		JMP		3F
2H		BOD		$5,1B
		SET		$45,1
3H		OR		$45,$45,$45

		% test BODB, #8000000000000000
		JMP		2F
1H		SET		$46,2
		JMP		3F
2H		BOD		$6,1B
		SET		$46,1
3H		OR		$46,$46,$46

		% test BNN, #FFFFFFFFFFFFFFFF
		BNN		$2,1F
		SET		$47,1
		JMP		2F
1H		SET		$47,2
2H		OR		$47,$47,$47

		% test BNN, #0
		BNN		$3,1F
		SET		$48,1
		JMP		2F
1H		SET		$48,2
2H		OR		$48,$48,$48

		% test BNN, #1
		BNN		$4,1F
		SET		$49,1
		JMP		2F
1H		SET		$49,2
2H		OR		$49,$49,$49

		% test BNN, #2
		BNN		$5,1F
		SET		$50,1
		JMP		2F
1H		SET		$50,2
2H		OR		$50,$50,$50

		% test BNN, #8000000000000000
		BNN		$6,1F
		SET		$51,1
		JMP		2F
1H		SET		$51,2
2H		OR		$51,$51,$51

		% test BNNB, #FFFFFFFFFFFFFFFF
		JMP		2F
1H		SET		$52,2
		JMP		3F
2H		BNN		$2,1B
		SET		$52,1
3H		OR		$52,$52,$52

		% test BNNB, #0
		JMP		2F
1H		SET		$53,2
		JMP		3F
2H		BNN		$3,1B
		SET		$53,1
3H		OR		$53,$53,$53

		% test BNNB, #1
		JMP		2F
1H		SET		$54,2
		JMP		3F
2H		BNN		$4,1B
		SET		$54,1
3H		OR		$54,$54,$54

		% test BNNB, #2
		JMP		2F
1H		SET		$55,2
		JMP		3F
2H		BNN		$5,1B
		SET		$55,1
3H		OR		$55,$55,$55

		% test BNNB, #8000000000000000
		JMP		2F
1H		SET		$56,2
		JMP		3F
2H		BNN		$6,1B
		SET		$56,1
3H		OR		$56,$56,$56

		% test BNZ, #FFFFFFFFFFFFFFFF
		BNZ		$2,1F
		SET		$57,1
		JMP		2F
1H		SET		$57,2
2H		OR		$57,$57,$57

		% test BNZ, #0
		BNZ		$3,1F
		SET		$58,1
		JMP		2F
1H		SET		$58,2
2H		OR		$58,$58,$58

		% test BNZ, #1
		BNZ		$4,1F
		SET		$59,1
		JMP		2F
1H		SET		$59,2
2H		OR		$59,$59,$59

		% test BNZ, #2
		BNZ		$5,1F
		SET		$60,1
		JMP		2F
1H		SET		$60,2
2H		OR		$60,$60,$60

		% test BNZ, #8000000000000000
		BNZ		$6,1F
		SET		$61,1
		JMP		2F
1H		SET		$61,2
2H		OR		$61,$61,$61

		% test BNZB, #FFFFFFFFFFFFFFFF
		JMP		2F
1H		SET		$62,2
		JMP		3F
2H		BNZ		$2,1B
		SET		$62,1
3H		OR		$62,$62,$62

		% test BNZB, #0
		JMP		2F
1H		SET		$63,2
		JMP		3F
2H		BNZ		$3,1B
		SET		$63,1
3H		OR		$63,$63,$63

		% test BNZB, #1
		JMP		2F
1H		SET		$64,2
		JMP		3F
2H		BNZ		$4,1B
		SET		$64,1
3H		OR		$64,$64,$64

		% test BNZB, #2
		JMP		2F
1H		SET		$65,2
		JMP		3F
2H		BNZ		$5,1B
		SET		$65,1
3H		OR		$65,$65,$65

		% test BNZB, #8000000000000000
		JMP		2F
1H		SET		$66,2
		JMP		3F
2H		BNZ		$6,1B
		SET		$66,1
3H		OR		$66,$66,$66

		% test BNP, #FFFFFFFFFFFFFFFF
		BNP		$2,1F
		SET		$67,1
		JMP		2F
1H		SET		$67,2
2H		OR		$67,$67,$67

		% test BNP, #0
		BNP		$3,1F
		SET		$68,1
		JMP		2F
1H		SET		$68,2
2H		OR		$68,$68,$68

		% test BNP, #1
		BNP		$4,1F
		SET		$69,1
		JMP		2F
1H		SET		$69,2
2H		OR		$69,$69,$69

		% test BNP, #2
		BNP		$5,1F
		SET		$70,1
		JMP		2F
1H		SET		$70,2
2H		OR		$70,$70,$70

		% test BNP, #8000000000000000
		BNP		$6,1F
		SET		$71,1
		JMP		2F
1H		SET		$71,2
2H		OR		$71,$71,$71

		% test BNPB, #FFFFFFFFFFFFFFFF
		JMP		2F
1H		SET		$72,2
		JMP		3F
2H		BNP		$2,1B
		SET		$72,1
3H		OR		$72,$72,$72

		% test BNPB, #0
		JMP		2F
1H		SET		$73,2
		JMP		3F
2H		BNP		$3,1B
		SET		$73,1
3H		OR		$73,$73,$73

		% test BNPB, #1
		JMP		2F
1H		SET		$74,2
		JMP		3F
2H		BNP		$4,1B
		SET		$74,1
3H		OR		$74,$74,$74

		% test BNPB, #2
		JMP		2F
1H		SET		$75,2
		JMP		3F
2H		BNP		$5,1B
		SET		$75,1
3H		OR		$75,$75,$75

		% test BNPB, #8000000000000000
		JMP		2F
1H		SET		$76,2
		JMP		3F
2H		BNP		$6,1B
		SET		$76,1
3H		OR		$76,$76,$76

		% test BEV, #FFFFFFFFFFFFFFFF
		BEV		$2,1F
		SET		$77,1
		JMP		2F
1H		SET		$77,2
2H		OR		$77,$77,$77

		% test BEV, #0
		BEV		$3,1F
		SET		$78,1
		JMP		2F
1H		SET		$78,2
2H		OR		$78,$78,$78

		% test BEV, #1
		BEV		$4,1F
		SET		$79,1
		JMP		2F
1H		SET		$79,2
2H		OR		$79,$79,$79

		% test BEV, #2
		BEV		$5,1F
		SET		$80,1
		JMP		2F
1H		SET		$80,2
2H		OR		$80,$80,$80

		% test BEV, #8000000000000000
		BEV		$6,1F
		SET		$81,1
		JMP		2F
1H		SET		$81,2
2H		OR		$81,$81,$81

		% test BEVB, #FFFFFFFFFFFFFFFF
		JMP		2F
1H		SET		$82,2
		JMP		3F
2H		BEV		$2,1B
		SET		$82,1
3H		OR		$82,$82,$82

		% test BEVB, #0
		JMP		2F
1H		SET		$83,2
		JMP		3F
2H		BEV		$3,1B
		SET		$83,1
3H		OR		$83,$83,$83

		% test BEVB, #1
		JMP		2F
1H		SET		$84,2
		JMP		3F
2H		BEV		$4,1B
		SET		$84,1
3H		OR		$84,$84,$84

		% test BEVB, #2
		JMP		2F
1H		SET		$85,2
		JMP		3F
2H		BEV		$5,1B
		SET		$85,1
3H		OR		$85,$85,$85

		% test BEVB, #8000000000000000
		JMP		2F
1H		SET		$86,2
		JMP		3F
2H		BEV		$6,1B
		SET		$86,1
3H		OR		$86,$86,$86

		TRAP	0
