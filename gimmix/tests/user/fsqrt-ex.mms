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
		GET		$26,rA
		STOU	$26,$24,8		% #F008
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$1		% (sqrt) -0.000000e+00
		STOU	$26,$24,0		% #F010
		GET		$26,rA
		STOU	$26,$24,8		% #F018
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$2		% (sqrt) 1.000000e+00
		STOU	$26,$24,0		% #F020
		GET		$26,rA
		STOU	$26,$24,8		% #F028
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$3		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F030
		GET		$26,rA
		STOU	$26,$24,8		% #F038
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$4		% (sqrt) -1.500000e+00
		STOU	$26,$24,0		% #F040
		GET		$26,rA
		STOU	$26,$24,8		% #F048
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$5		% (sqrt) -3.312300e-02
		STOU	$26,$24,0		% #F050
		GET		$26,rA
		STOU	$26,$24,8		% #F058
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$6		% (sqrt) 1.110000e+00
		STOU	$26,$24,0		% #F060
		GET		$26,rA
		STOU	$26,$24,8		% #F068
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$7		% (sqrt) 1.999999e+00
		STOU	$26,$24,0		% #F070
		GET		$26,rA
		STOU	$26,$24,8		% #F078
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$8		% (sqrt) 4.503600e+15
		STOU	$26,$24,0		% #F080
		GET		$26,rA
		STOU	$26,$24,8		% #F088
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$9		% (sqrt) -7.500000e-01
		STOU	$26,$24,0		% #F090
		GET		$26,rA
		STOU	$26,$24,8		% #F098
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$10		% (sqrt) -1.275932e+01
		STOU	$26,$24,0		% #F0A0
		GET		$26,rA
		STOU	$26,$24,8		% #F0A8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$11		% (sqrt) 7.771145e+02
		STOU	$26,$24,0		% #F0B0
		GET		$26,rA
		STOU	$26,$24,8		% #F0B8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$12		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F0C0
		GET		$26,rA
		STOU	$26,$24,8		% #F0C8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$13		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F0D0
		GET		$26,rA
		STOU	$26,$24,8		% #F0D8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$14		% (sqrt) 2.216416e-308
		STOU	$26,$24,0		% #F0E0
		GET		$26,rA
		STOU	$26,$24,8		% #F0E8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$15		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F0F0
		GET		$26,rA
		STOU	$26,$24,8		% #F0F8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$16		% (sqrt) 1.797693e+308
		STOU	$26,$24,0		% #F100
		GET		$26,rA
		STOU	$26,$24,8		% #F108
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$17		% (sqrt) -1.797693e+308
		STOU	$26,$24,0		% #F110
		GET		$26,rA
		STOU	$26,$24,8		% #F118
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$18		% (sqrt) 4.940656e-324
		STOU	$26,$24,0		% #F120
		GET		$26,rA
		STOU	$26,$24,8		% #F128
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$19		% (sqrt) -4.940656e-324
		STOU	$26,$24,0		% #F130
		GET		$26,rA
		STOU	$26,$24,8		% #F138
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$20		% (sqrt) inf
		STOU	$26,$24,0		% #F140
		GET		$26,rA
		STOU	$26,$24,8		% #F148
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$21		% (sqrt) -inf
		STOU	$26,$24,0		% #F150
		GET		$26,rA
		STOU	$26,$24,8		% #F158
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$22		% (sqrt) nan
		STOU	$26,$24,0		% #F160
		GET		$26,rA
		STOU	$26,$24,8		% #F168
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$23		% (sqrt) nan
		STOU	$26,$24,0		% #F170
		GET		$26,rA
		STOU	$26,$24,8		% #F178
		PUT		rA,$25
		ADDU	$24,$24,16

		% Set rounding mode to DOWN
		SETML	$25,#3
		PUT		rA,$25

		FSQRT	$26,$0		% (sqrt) 0.000000e+00
		STOU	$26,$24,0		% #F180
		GET		$26,rA
		STOU	$26,$24,8		% #F188
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$1		% (sqrt) -0.000000e+00
		STOU	$26,$24,0		% #F190
		GET		$26,rA
		STOU	$26,$24,8		% #F198
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$2		% (sqrt) 1.000000e+00
		STOU	$26,$24,0		% #F1A0
		GET		$26,rA
		STOU	$26,$24,8		% #F1A8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$3		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F1B0
		GET		$26,rA
		STOU	$26,$24,8		% #F1B8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$4		% (sqrt) -1.500000e+00
		STOU	$26,$24,0		% #F1C0
		GET		$26,rA
		STOU	$26,$24,8		% #F1C8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$5		% (sqrt) -3.312300e-02
		STOU	$26,$24,0		% #F1D0
		GET		$26,rA
		STOU	$26,$24,8		% #F1D8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$6		% (sqrt) 1.110000e+00
		STOU	$26,$24,0		% #F1E0
		GET		$26,rA
		STOU	$26,$24,8		% #F1E8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$7		% (sqrt) 1.999999e+00
		STOU	$26,$24,0		% #F1F0
		GET		$26,rA
		STOU	$26,$24,8		% #F1F8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$8		% (sqrt) 4.503600e+15
		STOU	$26,$24,0		% #F200
		GET		$26,rA
		STOU	$26,$24,8		% #F208
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$9		% (sqrt) -7.500000e-01
		STOU	$26,$24,0		% #F210
		GET		$26,rA
		STOU	$26,$24,8		% #F218
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$10		% (sqrt) -1.275932e+01
		STOU	$26,$24,0		% #F220
		GET		$26,rA
		STOU	$26,$24,8		% #F228
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$11		% (sqrt) 7.771145e+02
		STOU	$26,$24,0		% #F230
		GET		$26,rA
		STOU	$26,$24,8		% #F238
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$12		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F240
		GET		$26,rA
		STOU	$26,$24,8		% #F248
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$13		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F250
		GET		$26,rA
		STOU	$26,$24,8		% #F258
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$14		% (sqrt) 2.216416e-308
		STOU	$26,$24,0		% #F260
		GET		$26,rA
		STOU	$26,$24,8		% #F268
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$15		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F270
		GET		$26,rA
		STOU	$26,$24,8		% #F278
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$16		% (sqrt) 1.797693e+308
		STOU	$26,$24,0		% #F280
		GET		$26,rA
		STOU	$26,$24,8		% #F288
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$17		% (sqrt) -1.797693e+308
		STOU	$26,$24,0		% #F290
		GET		$26,rA
		STOU	$26,$24,8		% #F298
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$18		% (sqrt) 4.940656e-324
		STOU	$26,$24,0		% #F2A0
		GET		$26,rA
		STOU	$26,$24,8		% #F2A8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$19		% (sqrt) -4.940656e-324
		STOU	$26,$24,0		% #F2B0
		GET		$26,rA
		STOU	$26,$24,8		% #F2B8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$20		% (sqrt) inf
		STOU	$26,$24,0		% #F2C0
		GET		$26,rA
		STOU	$26,$24,8		% #F2C8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$21		% (sqrt) -inf
		STOU	$26,$24,0		% #F2D0
		GET		$26,rA
		STOU	$26,$24,8		% #F2D8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$22		% (sqrt) nan
		STOU	$26,$24,0		% #F2E0
		GET		$26,rA
		STOU	$26,$24,8		% #F2E8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$23		% (sqrt) nan
		STOU	$26,$24,0		% #F2F0
		GET		$26,rA
		STOU	$26,$24,8		% #F2F8
		PUT		rA,$25
		ADDU	$24,$24,16

		% Set rounding mode to UP
		SETML	$25,#2
		PUT		rA,$25

		FSQRT	$26,$0		% (sqrt) 0.000000e+00
		STOU	$26,$24,0		% #F300
		GET		$26,rA
		STOU	$26,$24,8		% #F308
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$1		% (sqrt) -0.000000e+00
		STOU	$26,$24,0		% #F310
		GET		$26,rA
		STOU	$26,$24,8		% #F318
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$2		% (sqrt) 1.000000e+00
		STOU	$26,$24,0		% #F320
		GET		$26,rA
		STOU	$26,$24,8		% #F328
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$3		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F330
		GET		$26,rA
		STOU	$26,$24,8		% #F338
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$4		% (sqrt) -1.500000e+00
		STOU	$26,$24,0		% #F340
		GET		$26,rA
		STOU	$26,$24,8		% #F348
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$5		% (sqrt) -3.312300e-02
		STOU	$26,$24,0		% #F350
		GET		$26,rA
		STOU	$26,$24,8		% #F358
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$6		% (sqrt) 1.110000e+00
		STOU	$26,$24,0		% #F360
		GET		$26,rA
		STOU	$26,$24,8		% #F368
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$7		% (sqrt) 1.999999e+00
		STOU	$26,$24,0		% #F370
		GET		$26,rA
		STOU	$26,$24,8		% #F378
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$8		% (sqrt) 4.503600e+15
		STOU	$26,$24,0		% #F380
		GET		$26,rA
		STOU	$26,$24,8		% #F388
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$9		% (sqrt) -7.500000e-01
		STOU	$26,$24,0		% #F390
		GET		$26,rA
		STOU	$26,$24,8		% #F398
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$10		% (sqrt) -1.275932e+01
		STOU	$26,$24,0		% #F3A0
		GET		$26,rA
		STOU	$26,$24,8		% #F3A8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$11		% (sqrt) 7.771145e+02
		STOU	$26,$24,0		% #F3B0
		GET		$26,rA
		STOU	$26,$24,8		% #F3B8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$12		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F3C0
		GET		$26,rA
		STOU	$26,$24,8		% #F3C8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$13		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F3D0
		GET		$26,rA
		STOU	$26,$24,8		% #F3D8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$14		% (sqrt) 2.216416e-308
		STOU	$26,$24,0		% #F3E0
		GET		$26,rA
		STOU	$26,$24,8		% #F3E8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$15		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F3F0
		GET		$26,rA
		STOU	$26,$24,8		% #F3F8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$16		% (sqrt) 1.797693e+308
		STOU	$26,$24,0		% #F400
		GET		$26,rA
		STOU	$26,$24,8		% #F408
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$17		% (sqrt) -1.797693e+308
		STOU	$26,$24,0		% #F410
		GET		$26,rA
		STOU	$26,$24,8		% #F418
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$18		% (sqrt) 4.940656e-324
		STOU	$26,$24,0		% #F420
		GET		$26,rA
		STOU	$26,$24,8		% #F428
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$19		% (sqrt) -4.940656e-324
		STOU	$26,$24,0		% #F430
		GET		$26,rA
		STOU	$26,$24,8		% #F438
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$20		% (sqrt) inf
		STOU	$26,$24,0		% #F440
		GET		$26,rA
		STOU	$26,$24,8		% #F448
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$21		% (sqrt) -inf
		STOU	$26,$24,0		% #F450
		GET		$26,rA
		STOU	$26,$24,8		% #F458
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$22		% (sqrt) nan
		STOU	$26,$24,0		% #F460
		GET		$26,rA
		STOU	$26,$24,8		% #F468
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$23		% (sqrt) nan
		STOU	$26,$24,0		% #F470
		GET		$26,rA
		STOU	$26,$24,8		% #F478
		PUT		rA,$25
		ADDU	$24,$24,16

		% Set rounding mode to ZERO
		SETML	$25,#1
		PUT		rA,$25

		FSQRT	$26,$0		% (sqrt) 0.000000e+00
		STOU	$26,$24,0		% #F480
		GET		$26,rA
		STOU	$26,$24,8		% #F488
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$1		% (sqrt) -0.000000e+00
		STOU	$26,$24,0		% #F490
		GET		$26,rA
		STOU	$26,$24,8		% #F498
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$2		% (sqrt) 1.000000e+00
		STOU	$26,$24,0		% #F4A0
		GET		$26,rA
		STOU	$26,$24,8		% #F4A8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$3		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F4B0
		GET		$26,rA
		STOU	$26,$24,8		% #F4B8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$4		% (sqrt) -1.500000e+00
		STOU	$26,$24,0		% #F4C0
		GET		$26,rA
		STOU	$26,$24,8		% #F4C8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$5		% (sqrt) -3.312300e-02
		STOU	$26,$24,0		% #F4D0
		GET		$26,rA
		STOU	$26,$24,8		% #F4D8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$6		% (sqrt) 1.110000e+00
		STOU	$26,$24,0		% #F4E0
		GET		$26,rA
		STOU	$26,$24,8		% #F4E8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$7		% (sqrt) 1.999999e+00
		STOU	$26,$24,0		% #F4F0
		GET		$26,rA
		STOU	$26,$24,8		% #F4F8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$8		% (sqrt) 4.503600e+15
		STOU	$26,$24,0		% #F500
		GET		$26,rA
		STOU	$26,$24,8		% #F508
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$9		% (sqrt) -7.500000e-01
		STOU	$26,$24,0		% #F510
		GET		$26,rA
		STOU	$26,$24,8		% #F518
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$10		% (sqrt) -1.275932e+01
		STOU	$26,$24,0		% #F520
		GET		$26,rA
		STOU	$26,$24,8		% #F528
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$11		% (sqrt) 7.771145e+02
		STOU	$26,$24,0		% #F530
		GET		$26,rA
		STOU	$26,$24,8		% #F538
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$12		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F540
		GET		$26,rA
		STOU	$26,$24,8		% #F548
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$13		% (sqrt) 2.225074e-308
		STOU	$26,$24,0		% #F550
		GET		$26,rA
		STOU	$26,$24,8		% #F558
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$14		% (sqrt) 2.216416e-308
		STOU	$26,$24,0		% #F560
		GET		$26,rA
		STOU	$26,$24,8		% #F568
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$15		% (sqrt) 2.000000e+00
		STOU	$26,$24,0		% #F570
		GET		$26,rA
		STOU	$26,$24,8		% #F578
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$16		% (sqrt) 1.797693e+308
		STOU	$26,$24,0		% #F580
		GET		$26,rA
		STOU	$26,$24,8		% #F588
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$17		% (sqrt) -1.797693e+308
		STOU	$26,$24,0		% #F590
		GET		$26,rA
		STOU	$26,$24,8		% #F598
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$18		% (sqrt) 4.940656e-324
		STOU	$26,$24,0		% #F5A0
		GET		$26,rA
		STOU	$26,$24,8		% #F5A8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$19		% (sqrt) -4.940656e-324
		STOU	$26,$24,0		% #F5B0
		GET		$26,rA
		STOU	$26,$24,8		% #F5B8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$20		% (sqrt) inf
		STOU	$26,$24,0		% #F5C0
		GET		$26,rA
		STOU	$26,$24,8		% #F5C8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$21		% (sqrt) -inf
		STOU	$26,$24,0		% #F5D0
		GET		$26,rA
		STOU	$26,$24,8		% #F5D8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$22		% (sqrt) nan
		STOU	$26,$24,0		% #F5E0
		GET		$26,rA
		STOU	$26,$24,8		% #F5E8
		PUT		rA,$25
		ADDU	$24,$24,16

		FSQRT	$26,$23		% (sqrt) nan
		STOU	$26,$24,0		% #F5F0
		GET		$26,rA
		STOU	$26,$24,8		% #F5F8
		PUT		rA,$25
		ADDU	$24,$24,16

		% Sync memory
		SETL	$24,#F000
		SYNCD	#FE,$24
		ADDU	$24,$24,#FF
		SYNCD	#FE,$24
		ADDU	$24,$24,#FF
		SYNCD	#FE,$24
		ADDU	$24,$24,#FF
		SYNCD	#FE,$24
		ADDU	$24,$24,#FF
		SYNCD	#FE,$24
		ADDU	$24,$24,#FF
		SYNCD	#FE,$24
		ADDU	$24,$24,#FF
		SYNCD	#C,$24
		ADDU	$24,$24,#D
