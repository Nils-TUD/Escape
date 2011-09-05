%
% shift.mms -- tests shift-instructions (generated)
%
		LOC		#1000

Main	NOR		$2,$2,$2	% = -1
		SETH	$3,#1234
		ORMH	$3,#5678
		ORML	$3,#90AB
		ORL		$3,#CDEF
	
		% == test SL(i) ==
		% $2 s<< 0
		SL		$4,$2,0
		SET		$5,0
		SL		$5,$2,$5
		GET		$6,rA
		PUT		rA,0

		% $3 s<< 0
		SL		$7,$3,0
		SET		$8,0
		SL		$8,$3,$8
		GET		$9,rA
		PUT		rA,0

		% $2 s<< 1
		SL		$10,$2,1
		SET		$11,1
		SL		$11,$2,$11
		GET		$12,rA
		PUT		rA,0

		% $3 s<< 1
		SL		$13,$3,1
		SET		$14,1
		SL		$14,$3,$14
		GET		$15,rA
		PUT		rA,0

		% $2 s<< 4
		SL		$16,$2,4
		SET		$17,4
		SL		$17,$2,$17
		GET		$18,rA
		PUT		rA,0

		% $3 s<< 4
		SL		$19,$3,4
		SET		$20,4
		SL		$20,$3,$20
		GET		$21,rA
		PUT		rA,0

		% $2 s<< 63
		SL		$22,$2,63
		SET		$23,63
		SL		$23,$2,$23
		GET		$24,rA
		PUT		rA,0

		% $3 s<< 63
		SL		$25,$3,63
		SET		$26,63
		SL		$26,$3,$26
		GET		$27,rA
		PUT		rA,0

		% $2 s<< 64
		SL		$28,$2,64
		SET		$29,64
		SL		$29,$2,$29
		GET		$30,rA
		PUT		rA,0

		% $3 s<< 64
		SL		$31,$3,64
		SET		$32,64
		SL		$32,$3,$32
		GET		$33,rA
		PUT		rA,0

		% $2 s<< 65
		SL		$34,$2,65
		SET		$35,65
		SL		$35,$2,$35
		GET		$36,rA
		PUT		rA,0

		% $3 s<< 65
		SL		$37,$3,65
		SET		$38,65
		SL		$38,$3,$38
		GET		$39,rA
		PUT		rA,0

		% $2 s<< 128
		SL		$40,$2,128
		SET		$41,128
		SL		$41,$2,$41
		GET		$42,rA
		PUT		rA,0

		% $3 s<< 128
		SL		$43,$3,128
		SET		$44,128
		SL		$44,$3,$44
		GET		$45,rA
		PUT		rA,0

		% == test SLU(i) ==
		% $2 u<< 0
		SLU		$46,$2,0
		SET		$47,0
		SLU		$47,$2,$47
		GET		$48,rA
		PUT		rA,0

		% $3 u<< 0
		SLU		$49,$3,0
		SET		$50,0
		SLU		$50,$3,$50
		GET		$51,rA
		PUT		rA,0

		% $2 u<< 1
		SLU		$52,$2,1
		SET		$53,1
		SLU		$53,$2,$53
		GET		$54,rA
		PUT		rA,0

		% $3 u<< 1
		SLU		$55,$3,1
		SET		$56,1
		SLU		$56,$3,$56
		GET		$57,rA
		PUT		rA,0

		% $2 u<< 4
		SLU		$58,$2,4
		SET		$59,4
		SLU		$59,$2,$59
		GET		$60,rA
		PUT		rA,0

		% $3 u<< 4
		SLU		$61,$3,4
		SET		$62,4
		SLU		$62,$3,$62
		GET		$63,rA
		PUT		rA,0

		% $2 u<< 63
		SLU		$64,$2,63
		SET		$65,63
		SLU		$65,$2,$65
		GET		$66,rA
		PUT		rA,0

		% $3 u<< 63
		SLU		$67,$3,63
		SET		$68,63
		SLU		$68,$3,$68
		GET		$69,rA
		PUT		rA,0

		% $2 u<< 64
		SLU		$70,$2,64
		SET		$71,64
		SLU		$71,$2,$71
		GET		$72,rA
		PUT		rA,0

		% $3 u<< 64
		SLU		$73,$3,64
		SET		$74,64
		SLU		$74,$3,$74
		GET		$75,rA
		PUT		rA,0

		% $2 u<< 65
		SLU		$76,$2,65
		SET		$77,65
		SLU		$77,$2,$77
		GET		$78,rA
		PUT		rA,0

		% $3 u<< 65
		SLU		$79,$3,65
		SET		$80,65
		SLU		$80,$3,$80
		GET		$81,rA
		PUT		rA,0

		% $2 u<< 128
		SLU		$82,$2,128
		SET		$83,128
		SLU		$83,$2,$83
		GET		$84,rA
		PUT		rA,0

		% $3 u<< 128
		SLU		$85,$3,128
		SET		$86,128
		SLU		$86,$3,$86
		GET		$87,rA
		PUT		rA,0

		% == test SR(i) ==
		% $2 s>> 0
		SR		$88,$2,0
		SET		$89,0
		SR		$89,$2,$89
		GET		$90,rA
		PUT		rA,0

		% $3 s>> 0
		SR		$91,$3,0
		SET		$92,0
		SR		$92,$3,$92
		GET		$93,rA
		PUT		rA,0

		% $2 s>> 1
		SR		$94,$2,1
		SET		$95,1
		SR		$95,$2,$95
		GET		$96,rA
		PUT		rA,0

		% $3 s>> 1
		SR		$97,$3,1
		SET		$98,1
		SR		$98,$3,$98
		GET		$99,rA
		PUT		rA,0

		% $2 s>> 4
		SR		$100,$2,4
		SET		$101,4
		SR		$101,$2,$101
		GET		$102,rA
		PUT		rA,0

		% $3 s>> 4
		SR		$103,$3,4
		SET		$104,4
		SR		$104,$3,$104
		GET		$105,rA
		PUT		rA,0

		% $2 s>> 63
		SR		$106,$2,63
		SET		$107,63
		SR		$107,$2,$107
		GET		$108,rA
		PUT		rA,0

		% $3 s>> 63
		SR		$109,$3,63
		SET		$110,63
		SR		$110,$3,$110
		GET		$111,rA
		PUT		rA,0

		% $2 s>> 64
		SR		$112,$2,64
		SET		$113,64
		SR		$113,$2,$113
		GET		$114,rA
		PUT		rA,0

		% $3 s>> 64
		SR		$115,$3,64
		SET		$116,64
		SR		$116,$3,$116
		GET		$117,rA
		PUT		rA,0

		% $2 s>> 65
		SR		$118,$2,65
		SET		$119,65
		SR		$119,$2,$119
		GET		$120,rA
		PUT		rA,0

		% $3 s>> 65
		SR		$121,$3,65
		SET		$122,65
		SR		$122,$3,$122
		GET		$123,rA
		PUT		rA,0

		% $2 s>> 128
		SR		$124,$2,128
		SET		$125,128
		SR		$125,$2,$125
		GET		$126,rA
		PUT		rA,0

		% $3 s>> 128
		SR		$127,$3,128
		SET		$128,128
		SR		$128,$3,$128
		GET		$129,rA
		PUT		rA,0

		% == test SRU(i) ==
		% $2 u>> 0
		SRU		$130,$2,0
		SET		$131,0
		SRU		$131,$2,$131
		GET		$132,rA
		PUT		rA,0

		% $3 u>> 0
		SRU		$133,$3,0
		SET		$134,0
		SRU		$134,$3,$134
		GET		$135,rA
		PUT		rA,0

		% $2 u>> 1
		SRU		$136,$2,1
		SET		$137,1
		SRU		$137,$2,$137
		GET		$138,rA
		PUT		rA,0

		% $3 u>> 1
		SRU		$139,$3,1
		SET		$140,1
		SRU		$140,$3,$140
		GET		$141,rA
		PUT		rA,0

		% $2 u>> 4
		SRU		$142,$2,4
		SET		$143,4
		SRU		$143,$2,$143
		GET		$144,rA
		PUT		rA,0

		% $3 u>> 4
		SRU		$145,$3,4
		SET		$146,4
		SRU		$146,$3,$146
		GET		$147,rA
		PUT		rA,0

		% $2 u>> 63
		SRU		$148,$2,63
		SET		$149,63
		SRU		$149,$2,$149
		GET		$150,rA
		PUT		rA,0

		% $3 u>> 63
		SRU		$151,$3,63
		SET		$152,63
		SRU		$152,$3,$152
		GET		$153,rA
		PUT		rA,0

		% $2 u>> 64
		SRU		$154,$2,64
		SET		$155,64
		SRU		$155,$2,$155
		GET		$156,rA
		PUT		rA,0

		% $3 u>> 64
		SRU		$157,$3,64
		SET		$158,64
		SRU		$158,$3,$158
		GET		$159,rA
		PUT		rA,0

		% $2 u>> 65
		SRU		$160,$2,65
		SET		$161,65
		SRU		$161,$2,$161
		GET		$162,rA
		PUT		rA,0

		% $3 u>> 65
		SRU		$163,$3,65
		SET		$164,65
		SRU		$164,$3,$164
		GET		$165,rA
		PUT		rA,0

		% $2 u>> 128
		SRU		$166,$2,128
		SET		$167,128
		SRU		$167,$2,$167
		GET		$168,rA
		PUT		rA,0

		% $3 u>> 128
		SRU		$169,$3,128
		SET		$170,128
		SRU		$170,$3,$170
		GET		$171,rA
		PUT		rA,0

		TRAP	0
