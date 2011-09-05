%
% fixu.mms -- tests fixu-instruction (generated)
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

		SETH	$15,#7FEF
		ORMH	$15,#FFFF
		ORML	$15,#FFFF
		ORL		$15,#FFFF

		SETH	$16,#FFEF
		ORMH	$16,#FFFF
		ORML	$16,#FFFF
		ORL		$16,#FFFF

		SETH	$17,#0000
		ORMH	$17,#0000
		ORML	$17,#0000
		ORL		$17,#0001

		SETH	$18,#8000
		ORMH	$18,#0000
		ORML	$18,#0000
		ORL		$18,#0001

		SETH	$19,#7FF0
		ORMH	$19,#0000
		ORML	$19,#0000
		ORL		$19,#0000

		SETH	$20,#FFF0
		ORMH	$20,#0000
		ORML	$20,#0000
		ORL		$20,#0000

		SETH	$21,#7FF8
		ORMH	$21,#0000
		ORML	$21,#0000
		ORL		$21,#0000

		SETH	$22,#7FF4
		ORMH	$22,#0000
		ORML	$22,#0000
		ORL		$22,#0000

		SETH	$23,#43E0
		ORMH	$23,#0000
		ORML	$23,#0000
		ORL		$23,#0000

		SETH	$24,#43E1
		ORMH	$24,#0000
		ORML	$24,#0000
		ORL		$24,#0000

		SETH	$25,#43DF
		ORMH	$25,#FFFF
		ORML	$25,#FFFF
		ORL		$25,#FFFF

		SETH	$26,#C3E0
		ORMH	$26,#0000
		ORML	$26,#0000
		ORL		$26,#0000

		SETH	$27,#C3E0
		ORMH	$27,#0000
		ORML	$27,#0000
		ORL		$27,#0001

		% Setup location for results
		SETL	$28,#F000

		% Perform tests
		% Set rounding mode to NEAR
		SETML	$29,#0
		PUT		rA,$29

		FIXU	$30,$0		% (fix) 0.000000e+00
		STOU	$30,$28,0		% #F000
		ADDU	$28,$28,8

		FIXU	$30,$1		% (fix) -0.000000e+00
		STOU	$30,$28,0		% #F008
		ADDU	$28,$28,8

		FIXU	$30,$2		% (fix) 1.000000e+00
		STOU	$30,$28,0		% #F010
		ADDU	$28,$28,8

		FIXU	$30,$3		% (fix) 2.000000e+00
		STOU	$30,$28,0		% #F018
		ADDU	$28,$28,8

		FIXU	$30,$4		% (fix) -1.500000e+00
		STOU	$30,$28,0		% #F020
		ADDU	$28,$28,8

		FIXU	$30,$5		% (fix) -3.312300e-02
		STOU	$30,$28,0		% #F028
		ADDU	$28,$28,8

		FIXU	$30,$6		% (fix) 1.110000e+00
		STOU	$30,$28,0		% #F030
		ADDU	$28,$28,8

		FIXU	$30,$7		% (fix) 1.999999e+00
		STOU	$30,$28,0		% #F038
		ADDU	$28,$28,8

		FIXU	$30,$8		% (fix) 4.503600e+15
		STOU	$30,$28,0		% #F040
		ADDU	$28,$28,8

		FIXU	$30,$9		% (fix) -7.500000e-01
		STOU	$30,$28,0		% #F048
		ADDU	$28,$28,8

		FIXU	$30,$10		% (fix) -1.275932e+01
		STOU	$30,$28,0		% #F050
		ADDU	$28,$28,8

		FIXU	$30,$11		% (fix) 7.771145e+02
		STOU	$30,$28,0		% #F058
		ADDU	$28,$28,8

		FIXU	$30,$12		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F060
		ADDU	$28,$28,8

		FIXU	$30,$13		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F068
		ADDU	$28,$28,8

		FIXU	$30,$14		% (fix) 2.216416e-308
		STOU	$30,$28,0		% #F070
		ADDU	$28,$28,8

		FIXU	$30,$15		% (fix) 1.797693e+308
		STOU	$30,$28,0		% #F078
		ADDU	$28,$28,8

		FIXU	$30,$16		% (fix) -1.797693e+308
		STOU	$30,$28,0		% #F080
		ADDU	$28,$28,8

		FIXU	$30,$17		% (fix) 4.940656e-324
		STOU	$30,$28,0		% #F088
		ADDU	$28,$28,8

		FIXU	$30,$18		% (fix) -4.940656e-324
		STOU	$30,$28,0		% #F090
		ADDU	$28,$28,8

		FIXU	$30,$19		% (fix) inf
		STOU	$30,$28,0		% #F098
		ADDU	$28,$28,8

		FIXU	$30,$20		% (fix) -inf
		STOU	$30,$28,0		% #F0A0
		ADDU	$28,$28,8

		FIXU	$30,$21		% (fix) nan
		STOU	$30,$28,0		% #F0A8
		ADDU	$28,$28,8

		FIXU	$30,$22		% (fix) nan
		STOU	$30,$28,0		% #F0B0
		ADDU	$28,$28,8

		FIXU	$30,$23		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F0B8
		ADDU	$28,$28,8

		FIXU	$30,$24		% (fix) 9.799833e+18
		STOU	$30,$28,0		% #F0C0
		ADDU	$28,$28,8

		FIXU	$30,$25		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F0C8
		ADDU	$28,$28,8

		FIXU	$30,$26		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F0D0
		ADDU	$28,$28,8

		FIXU	$30,$27		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F0D8
		ADDU	$28,$28,8

		% Set rounding mode to DOWN
		SETML	$29,#3
		PUT		rA,$29

		FIXU	$30,$0		% (fix) 0.000000e+00
		STOU	$30,$28,0		% #F0E0
		ADDU	$28,$28,8

		FIXU	$30,$1		% (fix) -0.000000e+00
		STOU	$30,$28,0		% #F0E8
		ADDU	$28,$28,8

		FIXU	$30,$2		% (fix) 1.000000e+00
		STOU	$30,$28,0		% #F0F0
		ADDU	$28,$28,8

		FIXU	$30,$3		% (fix) 2.000000e+00
		STOU	$30,$28,0		% #F0F8
		ADDU	$28,$28,8

		FIXU	$30,$4		% (fix) -1.500000e+00
		STOU	$30,$28,0		% #F100
		ADDU	$28,$28,8

		FIXU	$30,$5		% (fix) -3.312300e-02
		STOU	$30,$28,0		% #F108
		ADDU	$28,$28,8

		FIXU	$30,$6		% (fix) 1.110000e+00
		STOU	$30,$28,0		% #F110
		ADDU	$28,$28,8

		FIXU	$30,$7		% (fix) 1.999999e+00
		STOU	$30,$28,0		% #F118
		ADDU	$28,$28,8

		FIXU	$30,$8		% (fix) 4.503600e+15
		STOU	$30,$28,0		% #F120
		ADDU	$28,$28,8

		FIXU	$30,$9		% (fix) -7.500000e-01
		STOU	$30,$28,0		% #F128
		ADDU	$28,$28,8

		FIXU	$30,$10		% (fix) -1.275932e+01
		STOU	$30,$28,0		% #F130
		ADDU	$28,$28,8

		FIXU	$30,$11		% (fix) 7.771145e+02
		STOU	$30,$28,0		% #F138
		ADDU	$28,$28,8

		FIXU	$30,$12		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F140
		ADDU	$28,$28,8

		FIXU	$30,$13		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F148
		ADDU	$28,$28,8

		FIXU	$30,$14		% (fix) 2.216416e-308
		STOU	$30,$28,0		% #F150
		ADDU	$28,$28,8

		FIXU	$30,$15		% (fix) 1.797693e+308
		STOU	$30,$28,0		% #F158
		ADDU	$28,$28,8

		FIXU	$30,$16		% (fix) -1.797693e+308
		STOU	$30,$28,0		% #F160
		ADDU	$28,$28,8

		FIXU	$30,$17		% (fix) 4.940656e-324
		STOU	$30,$28,0		% #F168
		ADDU	$28,$28,8

		FIXU	$30,$18		% (fix) -4.940656e-324
		STOU	$30,$28,0		% #F170
		ADDU	$28,$28,8

		FIXU	$30,$19		% (fix) inf
		STOU	$30,$28,0		% #F178
		ADDU	$28,$28,8

		FIXU	$30,$20		% (fix) -inf
		STOU	$30,$28,0		% #F180
		ADDU	$28,$28,8

		FIXU	$30,$21		% (fix) nan
		STOU	$30,$28,0		% #F188
		ADDU	$28,$28,8

		FIXU	$30,$22		% (fix) nan
		STOU	$30,$28,0		% #F190
		ADDU	$28,$28,8

		FIXU	$30,$23		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F198
		ADDU	$28,$28,8

		FIXU	$30,$24		% (fix) 9.799833e+18
		STOU	$30,$28,0		% #F1A0
		ADDU	$28,$28,8

		FIXU	$30,$25		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F1A8
		ADDU	$28,$28,8

		FIXU	$30,$26		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F1B0
		ADDU	$28,$28,8

		FIXU	$30,$27		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F1B8
		ADDU	$28,$28,8

		% Set rounding mode to UP
		SETML	$29,#2
		PUT		rA,$29

		FIXU	$30,$0		% (fix) 0.000000e+00
		STOU	$30,$28,0		% #F1C0
		ADDU	$28,$28,8

		FIXU	$30,$1		% (fix) -0.000000e+00
		STOU	$30,$28,0		% #F1C8
		ADDU	$28,$28,8

		FIXU	$30,$2		% (fix) 1.000000e+00
		STOU	$30,$28,0		% #F1D0
		ADDU	$28,$28,8

		FIXU	$30,$3		% (fix) 2.000000e+00
		STOU	$30,$28,0		% #F1D8
		ADDU	$28,$28,8

		FIXU	$30,$4		% (fix) -1.500000e+00
		STOU	$30,$28,0		% #F1E0
		ADDU	$28,$28,8

		FIXU	$30,$5		% (fix) -3.312300e-02
		STOU	$30,$28,0		% #F1E8
		ADDU	$28,$28,8

		FIXU	$30,$6		% (fix) 1.110000e+00
		STOU	$30,$28,0		% #F1F0
		ADDU	$28,$28,8

		FIXU	$30,$7		% (fix) 1.999999e+00
		STOU	$30,$28,0		% #F1F8
		ADDU	$28,$28,8

		FIXU	$30,$8		% (fix) 4.503600e+15
		STOU	$30,$28,0		% #F200
		ADDU	$28,$28,8

		FIXU	$30,$9		% (fix) -7.500000e-01
		STOU	$30,$28,0		% #F208
		ADDU	$28,$28,8

		FIXU	$30,$10		% (fix) -1.275932e+01
		STOU	$30,$28,0		% #F210
		ADDU	$28,$28,8

		FIXU	$30,$11		% (fix) 7.771145e+02
		STOU	$30,$28,0		% #F218
		ADDU	$28,$28,8

		FIXU	$30,$12		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F220
		ADDU	$28,$28,8

		FIXU	$30,$13		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F228
		ADDU	$28,$28,8

		FIXU	$30,$14		% (fix) 2.216416e-308
		STOU	$30,$28,0		% #F230
		ADDU	$28,$28,8

		FIXU	$30,$15		% (fix) 1.797693e+308
		STOU	$30,$28,0		% #F238
		ADDU	$28,$28,8

		FIXU	$30,$16		% (fix) -1.797693e+308
		STOU	$30,$28,0		% #F240
		ADDU	$28,$28,8

		FIXU	$30,$17		% (fix) 4.940656e-324
		STOU	$30,$28,0		% #F248
		ADDU	$28,$28,8

		FIXU	$30,$18		% (fix) -4.940656e-324
		STOU	$30,$28,0		% #F250
		ADDU	$28,$28,8

		FIXU	$30,$19		% (fix) inf
		STOU	$30,$28,0		% #F258
		ADDU	$28,$28,8

		FIXU	$30,$20		% (fix) -inf
		STOU	$30,$28,0		% #F260
		ADDU	$28,$28,8

		FIXU	$30,$21		% (fix) nan
		STOU	$30,$28,0		% #F268
		ADDU	$28,$28,8

		FIXU	$30,$22		% (fix) nan
		STOU	$30,$28,0		% #F270
		ADDU	$28,$28,8

		FIXU	$30,$23		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F278
		ADDU	$28,$28,8

		FIXU	$30,$24		% (fix) 9.799833e+18
		STOU	$30,$28,0		% #F280
		ADDU	$28,$28,8

		FIXU	$30,$25		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F288
		ADDU	$28,$28,8

		FIXU	$30,$26		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F290
		ADDU	$28,$28,8

		FIXU	$30,$27		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F298
		ADDU	$28,$28,8

		% Set rounding mode to ZERO
		SETML	$29,#1
		PUT		rA,$29

		FIXU	$30,$0		% (fix) 0.000000e+00
		STOU	$30,$28,0		% #F2A0
		ADDU	$28,$28,8

		FIXU	$30,$1		% (fix) -0.000000e+00
		STOU	$30,$28,0		% #F2A8
		ADDU	$28,$28,8

		FIXU	$30,$2		% (fix) 1.000000e+00
		STOU	$30,$28,0		% #F2B0
		ADDU	$28,$28,8

		FIXU	$30,$3		% (fix) 2.000000e+00
		STOU	$30,$28,0		% #F2B8
		ADDU	$28,$28,8

		FIXU	$30,$4		% (fix) -1.500000e+00
		STOU	$30,$28,0		% #F2C0
		ADDU	$28,$28,8

		FIXU	$30,$5		% (fix) -3.312300e-02
		STOU	$30,$28,0		% #F2C8
		ADDU	$28,$28,8

		FIXU	$30,$6		% (fix) 1.110000e+00
		STOU	$30,$28,0		% #F2D0
		ADDU	$28,$28,8

		FIXU	$30,$7		% (fix) 1.999999e+00
		STOU	$30,$28,0		% #F2D8
		ADDU	$28,$28,8

		FIXU	$30,$8		% (fix) 4.503600e+15
		STOU	$30,$28,0		% #F2E0
		ADDU	$28,$28,8

		FIXU	$30,$9		% (fix) -7.500000e-01
		STOU	$30,$28,0		% #F2E8
		ADDU	$28,$28,8

		FIXU	$30,$10		% (fix) -1.275932e+01
		STOU	$30,$28,0		% #F2F0
		ADDU	$28,$28,8

		FIXU	$30,$11		% (fix) 7.771145e+02
		STOU	$30,$28,0		% #F2F8
		ADDU	$28,$28,8

		FIXU	$30,$12		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F300
		ADDU	$28,$28,8

		FIXU	$30,$13		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F308
		ADDU	$28,$28,8

		FIXU	$30,$14		% (fix) 2.216416e-308
		STOU	$30,$28,0		% #F310
		ADDU	$28,$28,8

		FIXU	$30,$15		% (fix) 1.797693e+308
		STOU	$30,$28,0		% #F318
		ADDU	$28,$28,8

		FIXU	$30,$16		% (fix) -1.797693e+308
		STOU	$30,$28,0		% #F320
		ADDU	$28,$28,8

		FIXU	$30,$17		% (fix) 4.940656e-324
		STOU	$30,$28,0		% #F328
		ADDU	$28,$28,8

		FIXU	$30,$18		% (fix) -4.940656e-324
		STOU	$30,$28,0		% #F330
		ADDU	$28,$28,8

		FIXU	$30,$19		% (fix) inf
		STOU	$30,$28,0		% #F338
		ADDU	$28,$28,8

		FIXU	$30,$20		% (fix) -inf
		STOU	$30,$28,0		% #F340
		ADDU	$28,$28,8

		FIXU	$30,$21		% (fix) nan
		STOU	$30,$28,0		% #F348
		ADDU	$28,$28,8

		FIXU	$30,$22		% (fix) nan
		STOU	$30,$28,0		% #F350
		ADDU	$28,$28,8

		FIXU	$30,$23		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F358
		ADDU	$28,$28,8

		FIXU	$30,$24		% (fix) 9.799833e+18
		STOU	$30,$28,0		% #F360
		ADDU	$28,$28,8

		FIXU	$30,$25		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F368
		ADDU	$28,$28,8

		FIXU	$30,$26		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F370
		ADDU	$28,$28,8

		FIXU	$30,$27		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F378
		ADDU	$28,$28,8

		% Sync memory
		SETL	$28,#F000
		SYNCD	#FE,$28
		ADDU	$28,$28,#FF
		SYNCD	#FE,$28
		ADDU	$28,$28,#FF
		SYNCD	#FE,$28
		ADDU	$28,$28,#FF
		SYNCD	#86,$28
		ADDU	$28,$28,#87
