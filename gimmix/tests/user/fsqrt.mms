%
% fsqrt.mms -- tests fsqrt-instruction (generated)
%
		LOC		#1000

Main	OR		$0,$0,$0	% dummy
		PUT		rA,0

		% Put floats in registers
		SETH	$0,#0000
		ORMH	$0,#0000
		ORML	$0,#0000
		ORL		$0,#0000

		SETH	$1,#8000
		ORMH	$1,#0000
		ORML	$1,#0000
		ORL		$1,#0000

		SETH	$2,#3FF0
		ORMH	$2,#0000
		ORML	$2,#0000
		ORL		$2,#0000

		SETH	$3,#4000
		ORMH	$3,#0000
		ORML	$3,#0000
		ORL		$3,#0000

		SETH	$4,#BFF8
		ORMH	$4,#0000
		ORML	$4,#0000
		ORL		$4,#0000

		SETH	$5,#BFA0
		ORMH	$5,#F57F
		ORML	$5,#737D
		ORL		$5,#A61E

		SETH	$6,#3FF1
		ORMH	$6,#C28F
		ORML	$6,#5C28
		ORL		$6,#F5C3

		SETH	$7,#3FFF
		ORMH	$7,#FFFE
		ORML	$7,#F390
		ORL		$7,#85F5

		SETH	$8,#432F
		ORMH	$8,#FFFF
		ORML	$8,#FFFF
		ORL		$8,#FFFE

		SETH	$9,#BFE8
		ORMH	$9,#0000
		ORML	$9,#0000
		ORL		$9,#0000

		SETH	$10,#C029
		ORMH	$10,#84C5
		ORML	$10,#974E
		ORL		$10,#65BF

		SETH	$11,#4088
		ORMH	$11,#48EA
		ORML	$11,#6D26
		ORL		$11,#7408

		SETH	$12,#0010
		ORMH	$12,#0000
		ORML	$12,#0000
		ORL		$12,#0000

		SETH	$13,#000F
		ORMH	$13,#FFFF
		ORML	$13,#FFFF
		ORL		$13,#FFFF

		SETH	$14,#000F
		ORMH	$14,#F00F
		ORML	$14,#F00F
		ORL		$14,#F00F

		SETH	$15,#3FFF
		ORMH	$15,#FFFF
		ORML	$15,#FFFF
		ORL		$15,#FFFF

		SETH	$16,#7FEF
		ORMH	$16,#FFFF
		ORML	$16,#FFFF
		ORL		$16,#FFFF

		SETH	$17,#FFEF
		ORMH	$17,#FFFF
		ORML	$17,#FFFF
		ORL		$17,#FFFF

		SETH	$18,#0000
		ORMH	$18,#0000
		ORML	$18,#0000
		ORL		$18,#0001

		SETH	$19,#8000
		ORMH	$19,#0000
		ORML	$19,#0000
		ORL		$19,#0001

		SETH	$20,#7FF0
		ORMH	$20,#0000
		ORML	$20,#0000
		ORL		$20,#0000

		SETH	$21,#FFF0
		ORMH	$21,#0000
		ORML	$21,#0000
		ORL		$21,#0000

		SETH	$22,#7FF8
		ORMH	$22,#0000
		ORML	$22,#0000
		ORL		$22,#0000

		SETH	$23,#7FF4
		ORMH	$23,#0000
		ORML	$23,#0000
		ORL		$23,#0000

		% Setup location for results
		SETL	$24,#F000

		% Perform tests
		% Set rounding mode to NEAR
		SETML	$25,#0
		PUT		rA,$25

		FSQRT	$26,$0		% (sqrt) 0.000000e+00
		STOU	$26,$24,0		% #F000
		ADDU	$24,$24,8

		FSQRT	$26,$1		% (sqrt) -0.000000e+00
		STOU	$26,$24,0		% #F008
		ADDU	$24,$24,8

		FSQRT	$26,$2		% (sqrt) 1.000000e+00
		STOU	$26,$24,0		% #F010
		ADDU	$24,$24,8

		FSQRT	$26,$3		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F018
		ADDU	$24,$24,8

		FSQRT	$26,$4		% (sqrt) -1.500000e+00
		STOU	$26,$24,0		% #F020
		ADDU	$24,$24,8

		FSQRT	$26,$5		% (sqrt) -3.312300e-02
		STOU	$26,$24,0		% #F028
		ADDU	$24,$24,8

		FSQRT	$26,$6		% (sqrt) 1.110000e+00
		STOU	$26,$24,0		% #F030
		ADDU	$24,$24,8

		FSQRT	$26,$7		% (sqrt) 1.999999e+00
		STOU	$26,$24,0		% #F038
		ADDU	$24,$24,8

		FSQRT	$26,$8		% (sqrt) 4.503600e+15
		STOU	$26,$24,0		% #F040
		ADDU	$24,$24,8

		FSQRT	$26,$9		% (sqrt) -7.500000e-01
		STOU	$26,$24,0		% #F048
		ADDU	$24,$24,8

		FSQRT	$26,$10		% (sqrt) -1.275932e+01
		STOU	$26,$24,0		% #F050
		ADDU	$24,$24,8

		FSQRT	$26,$11		% (sqrt) 7.771145e+02
		STOU	$26,$24,0		% #F058
		ADDU	$24,$24,8

		FSQRT	$26,$12		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F060
		ADDU	$24,$24,8

		FSQRT	$26,$13		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F068
		ADDU	$24,$24,8

		FSQRT	$26,$14		% (sqrt) 2.216416e-308
		STOU	$26,$24,0		% #F070
		ADDU	$24,$24,8

		FSQRT	$26,$15		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F078
		ADDU	$24,$24,8

		FSQRT	$26,$16		% (sqrt) 1.797693e+308
		STOU	$26,$24,0		% #F080
		ADDU	$24,$24,8

		FSQRT	$26,$17		% (sqrt) -1.797693e+308
		STOU	$26,$24,0		% #F088
		ADDU	$24,$24,8

		FSQRT	$26,$18		% (sqrt) 4.940656e-324
		STOU	$26,$24,0		% #F090
		ADDU	$24,$24,8

		FSQRT	$26,$19		% (sqrt) -4.940656e-324
		STOU	$26,$24,0		% #F098
		ADDU	$24,$24,8

		FSQRT	$26,$20		% (sqrt) inf
		STOU	$26,$24,0		% #F0A0
		ADDU	$24,$24,8

		FSQRT	$26,$21		% (sqrt) -inf
		STOU	$26,$24,0		% #F0A8
		ADDU	$24,$24,8

		FSQRT	$26,$22		% (sqrt) nan
		STOU	$26,$24,0		% #F0B0
		ADDU	$24,$24,8

		FSQRT	$26,$23		% (sqrt) nan
		STOU	$26,$24,0		% #F0B8
		ADDU	$24,$24,8

		% Set rounding mode to DOWN
		SETML	$25,#3
		PUT		rA,$25

		FSQRT	$26,$0		% (sqrt) 0.000000e+00
		STOU	$26,$24,0		% #F0C0
		ADDU	$24,$24,8

		FSQRT	$26,$1		% (sqrt) -0.000000e+00
		STOU	$26,$24,0		% #F0C8
		ADDU	$24,$24,8

		FSQRT	$26,$2		% (sqrt) 1.000000e+00
		STOU	$26,$24,0		% #F0D0
		ADDU	$24,$24,8

		FSQRT	$26,$3		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F0D8
		ADDU	$24,$24,8

		FSQRT	$26,$4		% (sqrt) -1.500000e+00
		STOU	$26,$24,0		% #F0E0
		ADDU	$24,$24,8

		FSQRT	$26,$5		% (sqrt) -3.312300e-02
		STOU	$26,$24,0		% #F0E8
		ADDU	$24,$24,8

		FSQRT	$26,$6		% (sqrt) 1.110000e+00
		STOU	$26,$24,0		% #F0F0
		ADDU	$24,$24,8

		FSQRT	$26,$7		% (sqrt) 1.999999e+00
		STOU	$26,$24,0		% #F0F8
		ADDU	$24,$24,8

		FSQRT	$26,$8		% (sqrt) 4.503600e+15
		STOU	$26,$24,0		% #F100
		ADDU	$24,$24,8

		FSQRT	$26,$9		% (sqrt) -7.500000e-01
		STOU	$26,$24,0		% #F108
		ADDU	$24,$24,8

		FSQRT	$26,$10		% (sqrt) -1.275932e+01
		STOU	$26,$24,0		% #F110
		ADDU	$24,$24,8

		FSQRT	$26,$11		% (sqrt) 7.771145e+02
		STOU	$26,$24,0		% #F118
		ADDU	$24,$24,8

		FSQRT	$26,$12		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F120
		ADDU	$24,$24,8

		FSQRT	$26,$13		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F128
		ADDU	$24,$24,8

		FSQRT	$26,$14		% (sqrt) 2.216416e-308
		STOU	$26,$24,0		% #F130
		ADDU	$24,$24,8

		FSQRT	$26,$15		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F138
		ADDU	$24,$24,8

		FSQRT	$26,$16		% (sqrt) 1.797693e+308
		STOU	$26,$24,0		% #F140
		ADDU	$24,$24,8

		FSQRT	$26,$17		% (sqrt) -1.797693e+308
		STOU	$26,$24,0		% #F148
		ADDU	$24,$24,8

		FSQRT	$26,$18		% (sqrt) 4.940656e-324
		STOU	$26,$24,0		% #F150
		ADDU	$24,$24,8

		FSQRT	$26,$19		% (sqrt) -4.940656e-324
		STOU	$26,$24,0		% #F158
		ADDU	$24,$24,8

		FSQRT	$26,$20		% (sqrt) inf
		STOU	$26,$24,0		% #F160
		ADDU	$24,$24,8

		FSQRT	$26,$21		% (sqrt) -inf
		STOU	$26,$24,0		% #F168
		ADDU	$24,$24,8

		FSQRT	$26,$22		% (sqrt) nan
		STOU	$26,$24,0		% #F170
		ADDU	$24,$24,8

		FSQRT	$26,$23		% (sqrt) nan
		STOU	$26,$24,0		% #F178
		ADDU	$24,$24,8

		% Set rounding mode to UP
		SETML	$25,#2
		PUT		rA,$25

		FSQRT	$26,$0		% (sqrt) 0.000000e+00
		STOU	$26,$24,0		% #F180
		ADDU	$24,$24,8

		FSQRT	$26,$1		% (sqrt) -0.000000e+00
		STOU	$26,$24,0		% #F188
		ADDU	$24,$24,8

		FSQRT	$26,$2		% (sqrt) 1.000000e+00
		STOU	$26,$24,0		% #F190
		ADDU	$24,$24,8

		FSQRT	$26,$3		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F198
		ADDU	$24,$24,8

		FSQRT	$26,$4		% (sqrt) -1.500000e+00
		STOU	$26,$24,0		% #F1A0
		ADDU	$24,$24,8

		FSQRT	$26,$5		% (sqrt) -3.312300e-02
		STOU	$26,$24,0		% #F1A8
		ADDU	$24,$24,8

		FSQRT	$26,$6		% (sqrt) 1.110000e+00
		STOU	$26,$24,0		% #F1B0
		ADDU	$24,$24,8

		FSQRT	$26,$7		% (sqrt) 1.999999e+00
		STOU	$26,$24,0		% #F1B8
		ADDU	$24,$24,8

		FSQRT	$26,$8		% (sqrt) 4.503600e+15
		STOU	$26,$24,0		% #F1C0
		ADDU	$24,$24,8

		FSQRT	$26,$9		% (sqrt) -7.500000e-01
		STOU	$26,$24,0		% #F1C8
		ADDU	$24,$24,8

		FSQRT	$26,$10		% (sqrt) -1.275932e+01
		STOU	$26,$24,0		% #F1D0
		ADDU	$24,$24,8

		FSQRT	$26,$11		% (sqrt) 7.771145e+02
		STOU	$26,$24,0		% #F1D8
		ADDU	$24,$24,8

		FSQRT	$26,$12		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F1E0
		ADDU	$24,$24,8

		FSQRT	$26,$13		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F1E8
		ADDU	$24,$24,8

		FSQRT	$26,$14		% (sqrt) 2.216416e-308
		STOU	$26,$24,0		% #F1F0
		ADDU	$24,$24,8

		FSQRT	$26,$15		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F1F8
		ADDU	$24,$24,8

		FSQRT	$26,$16		% (sqrt) 1.797693e+308
		STOU	$26,$24,0		% #F200
		ADDU	$24,$24,8

		FSQRT	$26,$17		% (sqrt) -1.797693e+308
		STOU	$26,$24,0		% #F208
		ADDU	$24,$24,8

		FSQRT	$26,$18		% (sqrt) 4.940656e-324
		STOU	$26,$24,0		% #F210
		ADDU	$24,$24,8

		FSQRT	$26,$19		% (sqrt) -4.940656e-324
		STOU	$26,$24,0		% #F218
		ADDU	$24,$24,8

		FSQRT	$26,$20		% (sqrt) inf
		STOU	$26,$24,0		% #F220
		ADDU	$24,$24,8

		FSQRT	$26,$21		% (sqrt) -inf
		STOU	$26,$24,0		% #F228
		ADDU	$24,$24,8

		FSQRT	$26,$22		% (sqrt) nan
		STOU	$26,$24,0		% #F230
		ADDU	$24,$24,8

		FSQRT	$26,$23		% (sqrt) nan
		STOU	$26,$24,0		% #F238
		ADDU	$24,$24,8

		% Set rounding mode to ZERO
		SETML	$25,#1
		PUT		rA,$25

		FSQRT	$26,$0		% (sqrt) 0.000000e+00
		STOU	$26,$24,0		% #F240
		ADDU	$24,$24,8

		FSQRT	$26,$1		% (sqrt) -0.000000e+00
		STOU	$26,$24,0		% #F248
		ADDU	$24,$24,8

		FSQRT	$26,$2		% (sqrt) 1.000000e+00
		STOU	$26,$24,0		% #F250
		ADDU	$24,$24,8

		FSQRT	$26,$3		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F258
		ADDU	$24,$24,8

		FSQRT	$26,$4		% (sqrt) -1.500000e+00
		STOU	$26,$24,0		% #F260
		ADDU	$24,$24,8

		FSQRT	$26,$5		% (sqrt) -3.312300e-02
		STOU	$26,$24,0		% #F268
		ADDU	$24,$24,8

		FSQRT	$26,$6		% (sqrt) 1.110000e+00
		STOU	$26,$24,0		% #F270
		ADDU	$24,$24,8

		FSQRT	$26,$7		% (sqrt) 1.999999e+00
		STOU	$26,$24,0		% #F278
		ADDU	$24,$24,8

		FSQRT	$26,$8		% (sqrt) 4.503600e+15
		STOU	$26,$24,0		% #F280
		ADDU	$24,$24,8

		FSQRT	$26,$9		% (sqrt) -7.500000e-01
		STOU	$26,$24,0		% #F288
		ADDU	$24,$24,8

		FSQRT	$26,$10		% (sqrt) -1.275932e+01
		STOU	$26,$24,0		% #F290
		ADDU	$24,$24,8

		FSQRT	$26,$11		% (sqrt) 7.771145e+02
		STOU	$26,$24,0		% #F298
		ADDU	$24,$24,8

		FSQRT	$26,$12		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F2A0
		ADDU	$24,$24,8

		FSQRT	$26,$13		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F2A8
		ADDU	$24,$24,8

		FSQRT	$26,$14		% (sqrt) 2.216416e-308
		STOU	$26,$24,0		% #F2B0
		ADDU	$24,$24,8

		FSQRT	$26,$15		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F2B8
		ADDU	$24,$24,8

		FSQRT	$26,$16		% (sqrt) 1.797693e+308
		STOU	$26,$24,0		% #F2C0
		ADDU	$24,$24,8

		FSQRT	$26,$17		% (sqrt) -1.797693e+308
		STOU	$26,$24,0		% #F2C8
		ADDU	$24,$24,8

		FSQRT	$26,$18		% (sqrt) 4.940656e-324
		STOU	$26,$24,0		% #F2D0
		ADDU	$24,$24,8

		FSQRT	$26,$19		% (sqrt) -4.940656e-324
		STOU	$26,$24,0		% #F2D8
		ADDU	$24,$24,8

		FSQRT	$26,$20		% (sqrt) inf
		STOU	$26,$24,0		% #F2E0
		ADDU	$24,$24,8

		FSQRT	$26,$21		% (sqrt) -inf
		STOU	$26,$24,0		% #F2E8
		ADDU	$24,$24,8

		FSQRT	$26,$22		% (sqrt) nan
		STOU	$26,$24,0		% #F2F0
		ADDU	$24,$24,8

		FSQRT	$26,$23		% (sqrt) nan
		STOU	$26,$24,0		% #F2F8
		ADDU	$24,$24,8

		% Sync memory
		SETL	$24,#F000
		SYNCD	#FE,$24
		ADDU	$24,$24,#FF
		SYNCD	#FE,$24
		ADDU	$24,$24,#FF
		SYNCD	#FE,$24
		ADDU	$24,$24,#FF
		SYNCD	#6,$24
		ADDU	$24,$24,#7
