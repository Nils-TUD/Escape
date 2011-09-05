%
% ldstsf.mms -- tests ldsf and stsf (generated)
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

		SETH	$12,#369F
		ORMH	$12,#F868
		ORML	$12,#BF4D
		ORL		$12,#956A

		SETH	$13,#369B
		ORMH	$13,#6735
		ORML	$13,#3642
		ORL		$13,#8011

		SETH	$14,#37BA
		ORMH	$14,#2239
		ORML	$14,#3B33
		ORL		$14,#036B

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
		SETL	$24,#E000
		SETL	$25,#F000

		% Perform tests
		% Set rounding mode to NEAR
		SETML	$26,#0
		PUT		rA,$26

		STSF	$0,$24,0		% 0.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F000
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 0.000000e+00
		STOU	$27,$25,0		% #F008
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F010
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$1,$24,0		% -0.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F018
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -0.000000e+00
		STOU	$27,$25,0		% #F020
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F028
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$2,$24,0		% 1.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F030
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.000000e+00
		STOU	$27,$25,0		% #F038
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F040
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$3,$24,0		% 2.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F048
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 2.000000e+00
		STOU	$27,$25,0		% #F050
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F058
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$4,$24,0		% -1.500000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F060
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -1.500000e+00
		STOU	$27,$25,0		% #F068
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F070
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$5,$24,0		% -3.312300e-02 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F078
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -3.312300e-02
		STOU	$27,$25,0		% #F080
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F088
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$6,$24,0		% 1.110000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F090
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.110000e+00
		STOU	$27,$25,0		% #F098
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F0A0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$7,$24,0		% 1.999999e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F0A8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.999999e+00
		STOU	$27,$25,0		% #F0B0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F0B8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$8,$24,0		% 4.503600e+15 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F0C0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 4.503600e+15
		STOU	$27,$25,0		% #F0C8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F0D0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$9,$24,0		% -7.500000e-01 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F0D8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -7.500000e-01
		STOU	$27,$25,0		% #F0E0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F0E8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$10,$24,0		% -1.275932e+01 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F0F0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -1.275932e+01
		STOU	$27,$25,0		% #F0F8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F100
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$11,$24,0		% 7.771145e+02 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F108
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 7.771145e+02
		STOU	$27,$25,0		% #F110
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F118
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$12,$24,0		% 1.400000e-45 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F120
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.400000e-45
		STOU	$27,$25,0		% #F128
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F130
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$13,$24,0		% 1.200000e-45 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F138
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.200000e-45
		STOU	$27,$25,0		% #F140
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F148
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$14,$24,0		% 3.000000e-40 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F150
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 3.000000e-40
		STOU	$27,$25,0		% #F158
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F160
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$15,$24,0		% 2.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F168
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 2.000000e+00
		STOU	$27,$25,0		% #F170
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F178
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$16,$24,0		% 1.797693e+308 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F180
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.797693e+308
		STOU	$27,$25,0		% #F188
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F190
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$17,$24,0		% -1.797693e+308 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F198
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -1.797693e+308
		STOU	$27,$25,0		% #F1A0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F1A8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$18,$24,0		% 4.940656e-324 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F1B0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 4.940656e-324
		STOU	$27,$25,0		% #F1B8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F1C0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$19,$24,0		% -4.940656e-324 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F1C8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -4.940656e-324
		STOU	$27,$25,0		% #F1D0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F1D8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$20,$24,0		% inf -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F1E0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- inf
		STOU	$27,$25,0		% #F1E8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F1F0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$21,$24,0		% -inf -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F1F8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -inf
		STOU	$27,$25,0		% #F200
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F208
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$22,$24,0		% nan -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F210
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- nan
		STOU	$27,$25,0		% #F218
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F220
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$23,$24,0		% nan -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F228
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- nan
		STOU	$27,$25,0		% #F230
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F238
		ADDU	$25,$25,8
		PUT		rA,$26

		% Set rounding mode to DOWN
		SETML	$26,#3
		PUT		rA,$26

		STSF	$0,$24,0		% 0.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F240
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 0.000000e+00
		STOU	$27,$25,0		% #F248
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F250
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$1,$24,0		% -0.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F258
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -0.000000e+00
		STOU	$27,$25,0		% #F260
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F268
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$2,$24,0		% 1.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F270
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.000000e+00
		STOU	$27,$25,0		% #F278
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F280
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$3,$24,0		% 2.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F288
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 2.000000e+00
		STOU	$27,$25,0		% #F290
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F298
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$4,$24,0		% -1.500000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F2A0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -1.500000e+00
		STOU	$27,$25,0		% #F2A8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F2B0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$5,$24,0		% -3.312300e-02 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F2B8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -3.312300e-02
		STOU	$27,$25,0		% #F2C0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F2C8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$6,$24,0		% 1.110000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F2D0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.110000e+00
		STOU	$27,$25,0		% #F2D8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F2E0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$7,$24,0		% 1.999999e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F2E8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.999999e+00
		STOU	$27,$25,0		% #F2F0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F2F8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$8,$24,0		% 4.503600e+15 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F300
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 4.503600e+15
		STOU	$27,$25,0		% #F308
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F310
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$9,$24,0		% -7.500000e-01 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F318
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -7.500000e-01
		STOU	$27,$25,0		% #F320
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F328
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$10,$24,0		% -1.275932e+01 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F330
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -1.275932e+01
		STOU	$27,$25,0		% #F338
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F340
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$11,$24,0		% 7.771145e+02 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F348
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 7.771145e+02
		STOU	$27,$25,0		% #F350
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F358
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$12,$24,0		% 1.400000e-45 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F360
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.400000e-45
		STOU	$27,$25,0		% #F368
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F370
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$13,$24,0		% 1.200000e-45 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F378
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.200000e-45
		STOU	$27,$25,0		% #F380
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F388
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$14,$24,0		% 3.000000e-40 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F390
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 3.000000e-40
		STOU	$27,$25,0		% #F398
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F3A0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$15,$24,0		% 2.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F3A8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 2.000000e+00
		STOU	$27,$25,0		% #F3B0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F3B8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$16,$24,0		% 1.797693e+308 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F3C0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.797693e+308
		STOU	$27,$25,0		% #F3C8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F3D0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$17,$24,0		% -1.797693e+308 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F3D8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -1.797693e+308
		STOU	$27,$25,0		% #F3E0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F3E8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$18,$24,0		% 4.940656e-324 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F3F0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 4.940656e-324
		STOU	$27,$25,0		% #F3F8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F400
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$19,$24,0		% -4.940656e-324 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F408
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -4.940656e-324
		STOU	$27,$25,0		% #F410
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F418
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$20,$24,0		% inf -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F420
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- inf
		STOU	$27,$25,0		% #F428
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F430
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$21,$24,0		% -inf -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F438
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -inf
		STOU	$27,$25,0		% #F440
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F448
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$22,$24,0		% nan -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F450
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- nan
		STOU	$27,$25,0		% #F458
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F460
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$23,$24,0		% nan -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F468
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- nan
		STOU	$27,$25,0		% #F470
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F478
		ADDU	$25,$25,8
		PUT		rA,$26

		% Set rounding mode to UP
		SETML	$26,#2
		PUT		rA,$26

		STSF	$0,$24,0		% 0.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F480
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 0.000000e+00
		STOU	$27,$25,0		% #F488
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F490
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$1,$24,0		% -0.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F498
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -0.000000e+00
		STOU	$27,$25,0		% #F4A0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F4A8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$2,$24,0		% 1.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F4B0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.000000e+00
		STOU	$27,$25,0		% #F4B8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F4C0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$3,$24,0		% 2.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F4C8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 2.000000e+00
		STOU	$27,$25,0		% #F4D0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F4D8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$4,$24,0		% -1.500000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F4E0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -1.500000e+00
		STOU	$27,$25,0		% #F4E8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F4F0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$5,$24,0		% -3.312300e-02 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F4F8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -3.312300e-02
		STOU	$27,$25,0		% #F500
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F508
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$6,$24,0		% 1.110000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F510
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.110000e+00
		STOU	$27,$25,0		% #F518
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F520
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$7,$24,0		% 1.999999e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F528
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.999999e+00
		STOU	$27,$25,0		% #F530
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F538
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$8,$24,0		% 4.503600e+15 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F540
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 4.503600e+15
		STOU	$27,$25,0		% #F548
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F550
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$9,$24,0		% -7.500000e-01 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F558
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -7.500000e-01
		STOU	$27,$25,0		% #F560
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F568
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$10,$24,0		% -1.275932e+01 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F570
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -1.275932e+01
		STOU	$27,$25,0		% #F578
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F580
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$11,$24,0		% 7.771145e+02 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F588
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 7.771145e+02
		STOU	$27,$25,0		% #F590
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F598
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$12,$24,0		% 1.400000e-45 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F5A0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.400000e-45
		STOU	$27,$25,0		% #F5A8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F5B0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$13,$24,0		% 1.200000e-45 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F5B8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.200000e-45
		STOU	$27,$25,0		% #F5C0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F5C8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$14,$24,0		% 3.000000e-40 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F5D0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 3.000000e-40
		STOU	$27,$25,0		% #F5D8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F5E0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$15,$24,0		% 2.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F5E8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 2.000000e+00
		STOU	$27,$25,0		% #F5F0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F5F8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$16,$24,0		% 1.797693e+308 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F600
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.797693e+308
		STOU	$27,$25,0		% #F608
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F610
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$17,$24,0		% -1.797693e+308 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F618
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -1.797693e+308
		STOU	$27,$25,0		% #F620
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F628
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$18,$24,0		% 4.940656e-324 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F630
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 4.940656e-324
		STOU	$27,$25,0		% #F638
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F640
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$19,$24,0		% -4.940656e-324 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F648
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -4.940656e-324
		STOU	$27,$25,0		% #F650
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F658
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$20,$24,0		% inf -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F660
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- inf
		STOU	$27,$25,0		% #F668
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F670
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$21,$24,0		% -inf -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F678
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -inf
		STOU	$27,$25,0		% #F680
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F688
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$22,$24,0		% nan -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F690
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- nan
		STOU	$27,$25,0		% #F698
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F6A0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$23,$24,0		% nan -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F6A8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- nan
		STOU	$27,$25,0		% #F6B0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F6B8
		ADDU	$25,$25,8
		PUT		rA,$26

		% Set rounding mode to ZERO
		SETML	$26,#1
		PUT		rA,$26

		STSF	$0,$24,0		% 0.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F6C0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 0.000000e+00
		STOU	$27,$25,0		% #F6C8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F6D0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$1,$24,0		% -0.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F6D8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -0.000000e+00
		STOU	$27,$25,0		% #F6E0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F6E8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$2,$24,0		% 1.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F6F0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.000000e+00
		STOU	$27,$25,0		% #F6F8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F700
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$3,$24,0		% 2.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F708
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 2.000000e+00
		STOU	$27,$25,0		% #F710
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F718
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$4,$24,0		% -1.500000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F720
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -1.500000e+00
		STOU	$27,$25,0		% #F728
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F730
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$5,$24,0		% -3.312300e-02 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F738
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -3.312300e-02
		STOU	$27,$25,0		% #F740
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F748
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$6,$24,0		% 1.110000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F750
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.110000e+00
		STOU	$27,$25,0		% #F758
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F760
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$7,$24,0		% 1.999999e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F768
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.999999e+00
		STOU	$27,$25,0		% #F770
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F778
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$8,$24,0		% 4.503600e+15 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F780
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 4.503600e+15
		STOU	$27,$25,0		% #F788
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F790
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$9,$24,0		% -7.500000e-01 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F798
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -7.500000e-01
		STOU	$27,$25,0		% #F7A0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F7A8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$10,$24,0		% -1.275932e+01 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F7B0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -1.275932e+01
		STOU	$27,$25,0		% #F7B8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F7C0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$11,$24,0		% 7.771145e+02 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F7C8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 7.771145e+02
		STOU	$27,$25,0		% #F7D0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F7D8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$12,$24,0		% 1.400000e-45 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F7E0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.400000e-45
		STOU	$27,$25,0		% #F7E8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F7F0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$13,$24,0		% 1.200000e-45 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F7F8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.200000e-45
		STOU	$27,$25,0		% #F800
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F808
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$14,$24,0		% 3.000000e-40 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F810
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 3.000000e-40
		STOU	$27,$25,0		% #F818
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F820
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$15,$24,0		% 2.000000e+00 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F828
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 2.000000e+00
		STOU	$27,$25,0		% #F830
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F838
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$16,$24,0		% 1.797693e+308 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F840
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 1.797693e+308
		STOU	$27,$25,0		% #F848
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F850
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$17,$24,0		% -1.797693e+308 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F858
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -1.797693e+308
		STOU	$27,$25,0		% #F860
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F868
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$18,$24,0		% 4.940656e-324 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F870
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- 4.940656e-324
		STOU	$27,$25,0		% #F878
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F880
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$19,$24,0		% -4.940656e-324 -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F888
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -4.940656e-324
		STOU	$27,$25,0		% #F890
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F898
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$20,$24,0		% inf -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F8A0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- inf
		STOU	$27,$25,0		% #F8A8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F8B0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$21,$24,0		% -inf -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F8B8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- -inf
		STOU	$27,$25,0		% #F8C0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F8C8
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$22,$24,0		% nan -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F8D0
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- nan
		STOU	$27,$25,0		% #F8D8
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F8E0
		ADDU	$25,$25,8
		PUT		rA,$26

		STSF	$23,$24,0		% nan -> #E000
		GET		$27,rA
		STOU	$27,$25,0		% #F8E8
		ADDU	$25,$25,8
		PUT		rA,$26
		LDSF	$27,$24,0		% $27 <- nan
		STOU	$27,$25,0		% #F8F0
		ADDU	$25,$25,8
		GET		$27,rA
		STOU	$27,$25,0		% #F8F8
		ADDU	$25,$25,8
		PUT		rA,$26

		% Sync memory
		SETL	$25,#F000
		SYNCD	#FE,$25
		ADDU	$25,$25,#FF
		SYNCD	#FE,$25
		ADDU	$25,$25,#FF
		SYNCD	#FE,$25
		ADDU	$25,$25,#FF
		SYNCD	#FE,$25
		ADDU	$25,$25,#FF
		SYNCD	#FE,$25
		ADDU	$25,$25,#FF
		SYNCD	#FE,$25
		ADDU	$25,$25,#FF
		SYNCD	#FE,$25
		ADDU	$25,$25,#FF
		SYNCD	#FE,$25
		ADDU	$25,$25,#FF
		SYNCD	#FE,$25
		ADDU	$25,$25,#FF
		SYNCD	#12,$25
		ADDU	$25,$25,#13
