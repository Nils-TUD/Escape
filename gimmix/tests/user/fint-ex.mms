%
% fint.mms -- tests fint-instruction (generated)
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

		FINT	$30,$0		% (int) 0.000000e+00
		STOU	$30,$28,0		% #F000
		GET		$30,rA
		STOU	$30,$28,8		% #F008
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$1		% (fix) -0.000000e+00
		STOU	$30,$28,0		% #F010
		GET		$30,rA
		STOU	$30,$28,8		% #F018
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$2		% (fix) 1.000000e+00
		STOU	$30,$28,0		% #F020
		GET		$30,rA
		STOU	$30,$28,8		% #F028
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$3		% (fix) 2.000000e+00
		STOU	$30,$28,0		% #F030
		GET		$30,rA
		STOU	$30,$28,8		% #F038
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$4		% (fix) -1.500000e+00
		STOU	$30,$28,0		% #F040
		GET		$30,rA
		STOU	$30,$28,8		% #F048
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$5		% (fix) -3.312300e-02
		STOU	$30,$28,0		% #F050
		GET		$30,rA
		STOU	$30,$28,8		% #F058
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$6		% (fix) 1.110000e+00
		STOU	$30,$28,0		% #F060
		GET		$30,rA
		STOU	$30,$28,8		% #F068
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$7		% (fix) 1.999999e+00
		STOU	$30,$28,0		% #F070
		GET		$30,rA
		STOU	$30,$28,8		% #F078
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$8		% (fix) 4.503600e+15
		STOU	$30,$28,0		% #F080
		GET		$30,rA
		STOU	$30,$28,8		% #F088
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$9		% (fix) -7.500000e-01
		STOU	$30,$28,0		% #F090
		GET		$30,rA
		STOU	$30,$28,8		% #F098
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$10		% (fix) -1.275932e+01
		STOU	$30,$28,0		% #F0A0
		GET		$30,rA
		STOU	$30,$28,8		% #F0A8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$11		% (fix) 7.771145e+02
		STOU	$30,$28,0		% #F0B0
		GET		$30,rA
		STOU	$30,$28,8		% #F0B8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$12		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F0C0
		GET		$30,rA
		STOU	$30,$28,8		% #F0C8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$13		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F0D0
		GET		$30,rA
		STOU	$30,$28,8		% #F0D8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$14		% (fix) 2.216416e-308
		STOU	$30,$28,0		% #F0E0
		GET		$30,rA
		STOU	$30,$28,8		% #F0E8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$15		% (fix) 1.797693e+308
		STOU	$30,$28,0		% #F0F0
		GET		$30,rA
		STOU	$30,$28,8		% #F0F8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$16		% (fix) -1.797693e+308
		STOU	$30,$28,0		% #F100
		GET		$30,rA
		STOU	$30,$28,8		% #F108
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$17		% (fix) 4.940656e-324
		STOU	$30,$28,0		% #F110
		GET		$30,rA
		STOU	$30,$28,8		% #F118
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$18		% (fix) -4.940656e-324
		STOU	$30,$28,0		% #F120
		GET		$30,rA
		STOU	$30,$28,8		% #F128
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$19		% (fix) inf
		STOU	$30,$28,0		% #F130
		GET		$30,rA
		STOU	$30,$28,8		% #F138
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$20		% (fix) -inf
		STOU	$30,$28,0		% #F140
		GET		$30,rA
		STOU	$30,$28,8		% #F148
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$21		% (fix) nan
		STOU	$30,$28,0		% #F150
		GET		$30,rA
		STOU	$30,$28,8		% #F158
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$22		% (fix) nan
		STOU	$30,$28,0		% #F160
		GET		$30,rA
		STOU	$30,$28,8		% #F168
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$23		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F170
		GET		$30,rA
		STOU	$30,$28,8		% #F178
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$24		% (fix) 9.799833e+18
		STOU	$30,$28,0		% #F180
		GET		$30,rA
		STOU	$30,$28,8		% #F188
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$25		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F190
		GET		$30,rA
		STOU	$30,$28,8		% #F198
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$26		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F1A0
		GET		$30,rA
		STOU	$30,$28,8		% #F1A8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$27		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F1B0
		GET		$30,rA
		STOU	$30,$28,8		% #F1B8
		PUT		rA,$29
		ADDU	$28,$28,16

		% Set rounding mode to DOWN
		SETML	$29,#3
		PUT		rA,$29

		FINT	$30,$0		% (fix) 0.000000e+00
		STOU	$30,$28,0		% #F1C0
		GET		$30,rA
		STOU	$30,$28,8		% #F1C8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$1		% (fix) -0.000000e+00
		STOU	$30,$28,0		% #F1D0
		GET		$30,rA
		STOU	$30,$28,8		% #F1D8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$2		% (fix) 1.000000e+00
		STOU	$30,$28,0		% #F1E0
		GET		$30,rA
		STOU	$30,$28,8		% #F1E8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$3		% (fix) 2.000000e+00
		STOU	$30,$28,0		% #F1F0
		GET		$30,rA
		STOU	$30,$28,8		% #F1F8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$4		% (fix) -1.500000e+00
		STOU	$30,$28,0		% #F200
		GET		$30,rA
		STOU	$30,$28,8		% #F208
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$5		% (fix) -3.312300e-02
		STOU	$30,$28,0		% #F210
		GET		$30,rA
		STOU	$30,$28,8		% #F218
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$6		% (fix) 1.110000e+00
		STOU	$30,$28,0		% #F220
		GET		$30,rA
		STOU	$30,$28,8		% #F228
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$7		% (fix) 1.999999e+00
		STOU	$30,$28,0		% #F230
		GET		$30,rA
		STOU	$30,$28,8		% #F238
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$8		% (fix) 4.503600e+15
		STOU	$30,$28,0		% #F240
		GET		$30,rA
		STOU	$30,$28,8		% #F248
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$9		% (fix) -7.500000e-01
		STOU	$30,$28,0		% #F250
		GET		$30,rA
		STOU	$30,$28,8		% #F258
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$10		% (fix) -1.275932e+01
		STOU	$30,$28,0		% #F260
		GET		$30,rA
		STOU	$30,$28,8		% #F268
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$11		% (fix) 7.771145e+02
		STOU	$30,$28,0		% #F270
		GET		$30,rA
		STOU	$30,$28,8		% #F278
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$12		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F280
		GET		$30,rA
		STOU	$30,$28,8		% #F288
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$13		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F290
		GET		$30,rA
		STOU	$30,$28,8		% #F298
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$14		% (fix) 2.216416e-308
		STOU	$30,$28,0		% #F2A0
		GET		$30,rA
		STOU	$30,$28,8		% #F2A8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$15		% (fix) 1.797693e+308
		STOU	$30,$28,0		% #F2B0
		GET		$30,rA
		STOU	$30,$28,8		% #F2B8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$16		% (fix) -1.797693e+308
		STOU	$30,$28,0		% #F2C0
		GET		$30,rA
		STOU	$30,$28,8		% #F2C8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$17		% (fix) 4.940656e-324
		STOU	$30,$28,0		% #F2D0
		GET		$30,rA
		STOU	$30,$28,8		% #F2D8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$18		% (fix) -4.940656e-324
		STOU	$30,$28,0		% #F2E0
		GET		$30,rA
		STOU	$30,$28,8		% #F2E8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$19		% (fix) inf
		STOU	$30,$28,0		% #F2F0
		GET		$30,rA
		STOU	$30,$28,8		% #F2F8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$20		% (fix) -inf
		STOU	$30,$28,0		% #F300
		GET		$30,rA
		STOU	$30,$28,8		% #F308
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$21		% (fix) nan
		STOU	$30,$28,0		% #F310
		GET		$30,rA
		STOU	$30,$28,8		% #F318
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$22		% (fix) nan
		STOU	$30,$28,0		% #F320
		GET		$30,rA
		STOU	$30,$28,8		% #F328
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$23		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F330
		GET		$30,rA
		STOU	$30,$28,8		% #F338
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$24		% (fix) 9.799833e+18
		STOU	$30,$28,0		% #F340
		GET		$30,rA
		STOU	$30,$28,8		% #F348
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$25		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F350
		GET		$30,rA
		STOU	$30,$28,8		% #F358
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$26		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F360
		GET		$30,rA
		STOU	$30,$28,8		% #F368
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$27		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F370
		GET		$30,rA
		STOU	$30,$28,8		% #F378
		PUT		rA,$29
		ADDU	$28,$28,16

		% Set rounding mode to UP
		SETML	$29,#2
		PUT		rA,$29

		FINT	$30,$0		% (fix) 0.000000e+00
		STOU	$30,$28,0		% #F380
		GET		$30,rA
		STOU	$30,$28,8		% #F388
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$1		% (fix) -0.000000e+00
		STOU	$30,$28,0		% #F390
		GET		$30,rA
		STOU	$30,$28,8		% #F398
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$2		% (fix) 1.000000e+00
		STOU	$30,$28,0		% #F3A0
		GET		$30,rA
		STOU	$30,$28,8		% #F3A8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$3		% (fix) 2.000000e+00
		STOU	$30,$28,0		% #F3B0
		GET		$30,rA
		STOU	$30,$28,8		% #F3B8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$4		% (fix) -1.500000e+00
		STOU	$30,$28,0		% #F3C0
		GET		$30,rA
		STOU	$30,$28,8		% #F3C8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$5		% (fix) -3.312300e-02
		STOU	$30,$28,0		% #F3D0
		GET		$30,rA
		STOU	$30,$28,8		% #F3D8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$6		% (fix) 1.110000e+00
		STOU	$30,$28,0		% #F3E0
		GET		$30,rA
		STOU	$30,$28,8		% #F3E8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$7		% (fix) 1.999999e+00
		STOU	$30,$28,0		% #F3F0
		GET		$30,rA
		STOU	$30,$28,8		% #F3F8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$8		% (fix) 4.503600e+15
		STOU	$30,$28,0		% #F400
		GET		$30,rA
		STOU	$30,$28,8		% #F408
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$9		% (fix) -7.500000e-01
		STOU	$30,$28,0		% #F410
		GET		$30,rA
		STOU	$30,$28,8		% #F418
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$10		% (fix) -1.275932e+01
		STOU	$30,$28,0		% #F420
		GET		$30,rA
		STOU	$30,$28,8		% #F428
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$11		% (fix) 7.771145e+02
		STOU	$30,$28,0		% #F430
		GET		$30,rA
		STOU	$30,$28,8		% #F438
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$12		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F440
		GET		$30,rA
		STOU	$30,$28,8		% #F448
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$13		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F450
		GET		$30,rA
		STOU	$30,$28,8		% #F458
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$14		% (fix) 2.216416e-308
		STOU	$30,$28,0		% #F460
		GET		$30,rA
		STOU	$30,$28,8		% #F468
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$15		% (fix) 1.797693e+308
		STOU	$30,$28,0		% #F470
		GET		$30,rA
		STOU	$30,$28,8		% #F478
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$16		% (fix) -1.797693e+308
		STOU	$30,$28,0		% #F480
		GET		$30,rA
		STOU	$30,$28,8		% #F488
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$17		% (fix) 4.940656e-324
		STOU	$30,$28,0		% #F490
		GET		$30,rA
		STOU	$30,$28,8		% #F498
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$18		% (fix) -4.940656e-324
		STOU	$30,$28,0		% #F4A0
		GET		$30,rA
		STOU	$30,$28,8		% #F4A8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$19		% (fix) inf
		STOU	$30,$28,0		% #F4B0
		GET		$30,rA
		STOU	$30,$28,8		% #F4B8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$20		% (fix) -inf
		STOU	$30,$28,0		% #F4C0
		GET		$30,rA
		STOU	$30,$28,8		% #F4C8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$21		% (fix) nan
		STOU	$30,$28,0		% #F4D0
		GET		$30,rA
		STOU	$30,$28,8		% #F4D8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$22		% (fix) nan
		STOU	$30,$28,0		% #F4E0
		GET		$30,rA
		STOU	$30,$28,8		% #F4E8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$23		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F4F0
		GET		$30,rA
		STOU	$30,$28,8		% #F4F8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$24		% (fix) 9.799833e+18
		STOU	$30,$28,0		% #F500
		GET		$30,rA
		STOU	$30,$28,8		% #F508
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$25		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F510
		GET		$30,rA
		STOU	$30,$28,8		% #F518
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$26		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F520
		GET		$30,rA
		STOU	$30,$28,8		% #F528
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$27		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F530
		GET		$30,rA
		STOU	$30,$28,8		% #F538
		PUT		rA,$29
		ADDU	$28,$28,16

		% Set rounding mode to ZERO
		SETML	$29,#1
		PUT		rA,$29

		FINT	$30,$0		% (fix) 0.000000e+00
		STOU	$30,$28,0		% #F540
		GET		$30,rA
		STOU	$30,$28,8		% #F548
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$1		% (fix) -0.000000e+00
		STOU	$30,$28,0		% #F550
		GET		$30,rA
		STOU	$30,$28,8		% #F558
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$2		% (fix) 1.000000e+00
		STOU	$30,$28,0		% #F560
		GET		$30,rA
		STOU	$30,$28,8		% #F568
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$3		% (fix) 2.000000e+00
		STOU	$30,$28,0		% #F570
		GET		$30,rA
		STOU	$30,$28,8		% #F578
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$4		% (fix) -1.500000e+00
		STOU	$30,$28,0		% #F580
		GET		$30,rA
		STOU	$30,$28,8		% #F588
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$5		% (fix) -3.312300e-02
		STOU	$30,$28,0		% #F590
		GET		$30,rA
		STOU	$30,$28,8		% #F598
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$6		% (fix) 1.110000e+00
		STOU	$30,$28,0		% #F5A0
		GET		$30,rA
		STOU	$30,$28,8		% #F5A8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$7		% (fix) 1.999999e+00
		STOU	$30,$28,0		% #F5B0
		GET		$30,rA
		STOU	$30,$28,8		% #F5B8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$8		% (fix) 4.503600e+15
		STOU	$30,$28,0		% #F5C0
		GET		$30,rA
		STOU	$30,$28,8		% #F5C8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$9		% (fix) -7.500000e-01
		STOU	$30,$28,0		% #F5D0
		GET		$30,rA
		STOU	$30,$28,8		% #F5D8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$10		% (fix) -1.275932e+01
		STOU	$30,$28,0		% #F5E0
		GET		$30,rA
		STOU	$30,$28,8		% #F5E8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$11		% (fix) 7.771145e+02
		STOU	$30,$28,0		% #F5F0
		GET		$30,rA
		STOU	$30,$28,8		% #F5F8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$12		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F600
		GET		$30,rA
		STOU	$30,$28,8		% #F608
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$13		% (fix) 2.225074e-308
		STOU	$30,$28,0		% #F610
		GET		$30,rA
		STOU	$30,$28,8		% #F618
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$14		% (fix) 2.216416e-308
		STOU	$30,$28,0		% #F620
		GET		$30,rA
		STOU	$30,$28,8		% #F628
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$15		% (fix) 1.797693e+308
		STOU	$30,$28,0		% #F630
		GET		$30,rA
		STOU	$30,$28,8		% #F638
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$16		% (fix) -1.797693e+308
		STOU	$30,$28,0		% #F640
		GET		$30,rA
		STOU	$30,$28,8		% #F648
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$17		% (fix) 4.940656e-324
		STOU	$30,$28,0		% #F650
		GET		$30,rA
		STOU	$30,$28,8		% #F658
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$18		% (fix) -4.940656e-324
		STOU	$30,$28,0		% #F660
		GET		$30,rA
		STOU	$30,$28,8		% #F668
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$19		% (fix) inf
		STOU	$30,$28,0		% #F670
		GET		$30,rA
		STOU	$30,$28,8		% #F678
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$20		% (fix) -inf
		STOU	$30,$28,0		% #F680
		GET		$30,rA
		STOU	$30,$28,8		% #F688
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$21		% (fix) nan
		STOU	$30,$28,0		% #F690
		GET		$30,rA
		STOU	$30,$28,8		% #F698
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$22		% (fix) nan
		STOU	$30,$28,0		% #F6A0
		GET		$30,rA
		STOU	$30,$28,8		% #F6A8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$23		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F6B0
		GET		$30,rA
		STOU	$30,$28,8		% #F6B8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$24		% (fix) 9.799833e+18
		STOU	$30,$28,0		% #F6C0
		GET		$30,rA
		STOU	$30,$28,8		% #F6C8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$25		% (fix) 9.223372e+18
		STOU	$30,$28,0		% #F6D0
		GET		$30,rA
		STOU	$30,$28,8		% #F6D8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$26		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F6E0
		GET		$30,rA
		STOU	$30,$28,8		% #F6E8
		PUT		rA,$29
		ADDU	$28,$28,16

		FINT	$30,$27		% (fix) -9.223372e+18
		STOU	$30,$28,0		% #F6F0
		GET		$30,rA
		STOU	$30,$28,8		% #F6F8
		PUT		rA,$29
		ADDU	$28,$28,16

		% Sync memory
		SETL	$28,#F000
		SYNCD	#FE,$28
		ADDU	$28,$28,#FF
		SYNCD	#FE,$28
		ADDU	$28,$28,#FF
		SYNCD	#FE,$28
		ADDU	$28,$28,#FF
		SYNCD	#FE,$28
		ADDU	$28,$28,#FF
		SYNCD	#FE,$28
		ADDU	$28,$28,#FF
		SYNCD	#FE,$28
		ADDU	$28,$28,#FF
		SYNCD	#FE,$28
		ADDU	$28,$28,#FF
		SYNCD	#E,$28
		ADDU	$28,$28,#F
