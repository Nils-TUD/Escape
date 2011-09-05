%
% store.mms -- tests store-instructions (generated)
%

		LOC		#1000

Main	SET		$0,#F000
		
		SETH	$2,#0000
		ORMH	$2,#0000
		ORML	$2,#0000
		ORL		$2,#0000
		SETH	$3,#0000
		ORMH	$3,#0000
		ORML	$3,#0000
		ORL		$3,#0001
		SETH	$4,#0000
		ORMH	$4,#0000
		ORML	$4,#0000
		ORL		$4,#000A
		SETH	$5,#0000
		ORMH	$5,#0000
		ORML	$5,#0000
		ORL		$5,#00FF
		SETH	$6,#0000
		ORMH	$6,#0000
		ORML	$6,#0000
		ORL		$6,#FFFF
		SETH	$7,#0000
		ORMH	$7,#0000
		ORML	$7,#FFFF
		ORL		$7,#FFFF
		SETH	$8,#FFFF
		ORMH	$8,#FFFF
		ORML	$8,#FFFF
		ORL		$8,#FFFF
		SETH	$9,#0000
		ORMH	$9,#0000
		ORML	$9,#0000
		ORL		$9,#007F
		SETH	$10,#0000
		ORMH	$10,#0000
		ORML	$10,#0000
		ORL		$10,#7FFF
		SETH	$11,#0000
		ORMH	$11,#0000
		ORML	$11,#7FFF
		ORL		$11,#FFFF
		SETH	$12,#7FFF
		ORMH	$12,#FFFF
		ORML	$12,#FFFF
		ORL		$12,#FFFF
		SETH	$13,#0000
		ORMH	$13,#0000
		ORML	$13,#0000
		ORL		$13,#0080
		SETH	$14,#0000
		ORMH	$14,#0000
		ORML	$14,#0000
		ORL		$14,#8000
		SETH	$15,#0000
		ORMH	$15,#0000
		ORML	$15,#8000
		ORL		$15,#0000
		SETH	$16,#8000
		ORMH	$16,#0000
		ORML	$16,#0000
		ORL		$16,#0000
		SETH	$17,#0000
		ORMH	$17,#0000
		ORML	$17,#0000
		ORL		$17,#007F
		SETH	$18,#0000
		ORMH	$18,#0000
		ORML	$18,#0000
		ORL		$18,#0080
		SETH	$19,#0000
		ORMH	$19,#0000
		ORML	$19,#0000
		ORL		$19,#7FFF
		SETH	$20,#0000
		ORMH	$20,#0000
		ORML	$20,#0000
		ORL		$20,#8000
		SETH	$21,#0000
		ORMH	$21,#0000
		ORML	$21,#7FFF
		ORL		$21,#FFFF
		SETH	$22,#0000
		ORMH	$22,#0000
		ORML	$22,#8000
		ORL		$22,#0000
		SETH	$23,#FFFF
		ORMH	$23,#FFFF
		ORML	$23,#FFFF
		ORL		$23,#FF80
		SETH	$24,#FFFF
		ORMH	$24,#FFFF
		ORML	$24,#FFFF
		ORL		$24,#FF7F
		SETH	$25,#FFFF
		ORMH	$25,#FFFF
		ORML	$25,#FFFF
		ORL		$25,#8000
		SETH	$26,#FFFF
		ORMH	$26,#FFFF
		ORML	$26,#FFFF
		ORL		$26,#7FFF
		SETH	$27,#FFFF
		ORMH	$27,#FFFF
		ORML	$27,#8000
		ORL		$27,#0000
		SETH	$28,#FFFF
		ORMH	$28,#FFFF
		ORML	$28,#7FFF
		ORL		$28,#FFFF

		% test STB(I)
		STB		$2,$0,0		% 0 -> #F000
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$3,$0,0		% 1 -> #F010
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$4,$0,0		% 10 -> #F020
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$5,$0,0		% 255 -> #F030
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$6,$0,0		% 65535 -> #F040
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$7,$0,0		% 4294967295 -> #F050
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$8,$0,0		% -1 -> #F060
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$9,$0,0		% 127 -> #F070
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$10,$0,0		% 32767 -> #F080
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$11,$0,0		% 2147483647 -> #F090
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$12,$0,0		% 9223372036854775807 -> #F0A0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$13,$0,0		% 128 -> #F0B0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$14,$0,0		% 32768 -> #F0C0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$15,$0,0		% 2147483648 -> #F0D0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$16,$0,0		% -9223372036854775808 -> #F0E0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$17,$0,0		% 127 -> #F0F0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$18,$0,0		% 128 -> #F100
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$19,$0,0		% 32767 -> #F110
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$20,$0,0		% 32768 -> #F120
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$21,$0,0		% 2147483647 -> #F130
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$22,$0,0		% 2147483648 -> #F140
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$23,$0,0		% -128 -> #F150
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$24,$0,0		% -129 -> #F160
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$25,$0,0		% -32768 -> #F170
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$26,$0,0		% -32769 -> #F180
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$27,$0,0		% -2147483648 -> #F190
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STB		$28,$0,0		% -2147483649 -> #F1A0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16


		% test STBU(I)
		STBU		$2,$0,0		% 0 -> #F1B0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$3,$0,0		% 1 -> #F1C0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$4,$0,0		% 10 -> #F1D0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$5,$0,0		% 255 -> #F1E0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$6,$0,0		% 65535 -> #F1F0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$7,$0,0		% 4294967295 -> #F200
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$8,$0,0		% -1 -> #F210
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$9,$0,0		% 127 -> #F220
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$10,$0,0		% 32767 -> #F230
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$11,$0,0		% 2147483647 -> #F240
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$12,$0,0		% 9223372036854775807 -> #F250
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$13,$0,0		% 128 -> #F260
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$14,$0,0		% 32768 -> #F270
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$15,$0,0		% 2147483648 -> #F280
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$16,$0,0		% -9223372036854775808 -> #F290
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$17,$0,0		% 127 -> #F2A0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$18,$0,0		% 128 -> #F2B0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$19,$0,0		% 32767 -> #F2C0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$20,$0,0		% 32768 -> #F2D0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$21,$0,0		% 2147483647 -> #F2E0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$22,$0,0		% 2147483648 -> #F2F0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$23,$0,0		% -128 -> #F300
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$24,$0,0		% -129 -> #F310
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$25,$0,0		% -32768 -> #F320
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$26,$0,0		% -32769 -> #F330
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$27,$0,0		% -2147483648 -> #F340
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STBU		$28,$0,0		% -2147483649 -> #F350
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16


		% test STW(I)
		STW		$2,$0,0		% 0 -> #F360
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$3,$0,0		% 1 -> #F370
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$4,$0,0		% 10 -> #F380
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$5,$0,0		% 255 -> #F390
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$6,$0,0		% 65535 -> #F3A0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$7,$0,0		% 4294967295 -> #F3B0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$8,$0,0		% -1 -> #F3C0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$9,$0,0		% 127 -> #F3D0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$10,$0,0		% 32767 -> #F3E0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$11,$0,0		% 2147483647 -> #F3F0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$12,$0,0		% 9223372036854775807 -> #F400
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$13,$0,0		% 128 -> #F410
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$14,$0,0		% 32768 -> #F420
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$15,$0,0		% 2147483648 -> #F430
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$16,$0,0		% -9223372036854775808 -> #F440
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$17,$0,0		% 127 -> #F450
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$18,$0,0		% 128 -> #F460
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$19,$0,0		% 32767 -> #F470
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$20,$0,0		% 32768 -> #F480
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$21,$0,0		% 2147483647 -> #F490
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$22,$0,0		% 2147483648 -> #F4A0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$23,$0,0		% -128 -> #F4B0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$24,$0,0		% -129 -> #F4C0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$25,$0,0		% -32768 -> #F4D0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$26,$0,0		% -32769 -> #F4E0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$27,$0,0		% -2147483648 -> #F4F0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STW		$28,$0,0		% -2147483649 -> #F500
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16


		% test STWU(I)
		STWU		$2,$0,0		% 0 -> #F510
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$3,$0,0		% 1 -> #F520
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$4,$0,0		% 10 -> #F530
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$5,$0,0		% 255 -> #F540
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$6,$0,0		% 65535 -> #F550
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$7,$0,0		% 4294967295 -> #F560
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$8,$0,0		% -1 -> #F570
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$9,$0,0		% 127 -> #F580
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$10,$0,0		% 32767 -> #F590
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$11,$0,0		% 2147483647 -> #F5A0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$12,$0,0		% 9223372036854775807 -> #F5B0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$13,$0,0		% 128 -> #F5C0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$14,$0,0		% 32768 -> #F5D0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$15,$0,0		% 2147483648 -> #F5E0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$16,$0,0		% -9223372036854775808 -> #F5F0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$17,$0,0		% 127 -> #F600
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$18,$0,0		% 128 -> #F610
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$19,$0,0		% 32767 -> #F620
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$20,$0,0		% 32768 -> #F630
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$21,$0,0		% 2147483647 -> #F640
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$22,$0,0		% 2147483648 -> #F650
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$23,$0,0		% -128 -> #F660
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$24,$0,0		% -129 -> #F670
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$25,$0,0		% -32768 -> #F680
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$26,$0,0		% -32769 -> #F690
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$27,$0,0		% -2147483648 -> #F6A0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STWU		$28,$0,0		% -2147483649 -> #F6B0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16


		% test STT(I)
		STT		$2,$0,0		% 0 -> #F6C0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$3,$0,0		% 1 -> #F6D0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$4,$0,0		% 10 -> #F6E0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$5,$0,0		% 255 -> #F6F0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$6,$0,0		% 65535 -> #F700
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$7,$0,0		% 4294967295 -> #F710
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$8,$0,0		% -1 -> #F720
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$9,$0,0		% 127 -> #F730
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$10,$0,0		% 32767 -> #F740
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$11,$0,0		% 2147483647 -> #F750
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$12,$0,0		% 9223372036854775807 -> #F760
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$13,$0,0		% 128 -> #F770
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$14,$0,0		% 32768 -> #F780
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$15,$0,0		% 2147483648 -> #F790
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$16,$0,0		% -9223372036854775808 -> #F7A0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$17,$0,0		% 127 -> #F7B0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$18,$0,0		% 128 -> #F7C0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$19,$0,0		% 32767 -> #F7D0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$20,$0,0		% 32768 -> #F7E0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$21,$0,0		% 2147483647 -> #F7F0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$22,$0,0		% 2147483648 -> #F800
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$23,$0,0		% -128 -> #F810
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$24,$0,0		% -129 -> #F820
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$25,$0,0		% -32768 -> #F830
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$26,$0,0		% -32769 -> #F840
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$27,$0,0		% -2147483648 -> #F850
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STT		$28,$0,0		% -2147483649 -> #F860
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16


		% test STTU(I)
		STTU		$2,$0,0		% 0 -> #F870
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$3,$0,0		% 1 -> #F880
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$4,$0,0		% 10 -> #F890
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$5,$0,0		% 255 -> #F8A0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$6,$0,0		% 65535 -> #F8B0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$7,$0,0		% 4294967295 -> #F8C0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$8,$0,0		% -1 -> #F8D0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$9,$0,0		% 127 -> #F8E0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$10,$0,0		% 32767 -> #F8F0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$11,$0,0		% 2147483647 -> #F900
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$12,$0,0		% 9223372036854775807 -> #F910
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$13,$0,0		% 128 -> #F920
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$14,$0,0		% 32768 -> #F930
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$15,$0,0		% 2147483648 -> #F940
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$16,$0,0		% -9223372036854775808 -> #F950
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$17,$0,0		% 127 -> #F960
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$18,$0,0		% 128 -> #F970
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$19,$0,0		% 32767 -> #F980
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$20,$0,0		% 32768 -> #F990
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$21,$0,0		% 2147483647 -> #F9A0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$22,$0,0		% 2147483648 -> #F9B0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$23,$0,0		% -128 -> #F9C0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$24,$0,0		% -129 -> #F9D0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$25,$0,0		% -32768 -> #F9E0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$26,$0,0		% -32769 -> #F9F0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$27,$0,0		% -2147483648 -> #FA00
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STTU		$28,$0,0		% -2147483649 -> #FA10
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16


		% test STO(I)
		STO		$2,$0,0		% 0 -> #FA20
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$3,$0,0		% 1 -> #FA30
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$4,$0,0		% 10 -> #FA40
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$5,$0,0		% 255 -> #FA50
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$6,$0,0		% 65535 -> #FA60
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$7,$0,0		% 4294967295 -> #FA70
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$8,$0,0		% -1 -> #FA80
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$9,$0,0		% 127 -> #FA90
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$10,$0,0		% 32767 -> #FAA0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$11,$0,0		% 2147483647 -> #FAB0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$12,$0,0		% 9223372036854775807 -> #FAC0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$13,$0,0		% 128 -> #FAD0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$14,$0,0		% 32768 -> #FAE0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$15,$0,0		% 2147483648 -> #FAF0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$16,$0,0		% -9223372036854775808 -> #FB00
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$17,$0,0		% 127 -> #FB10
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$18,$0,0		% 128 -> #FB20
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$19,$0,0		% 32767 -> #FB30
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$20,$0,0		% 32768 -> #FB40
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$21,$0,0		% 2147483647 -> #FB50
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$22,$0,0		% 2147483648 -> #FB60
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$23,$0,0		% -128 -> #FB70
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$24,$0,0		% -129 -> #FB80
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$25,$0,0		% -32768 -> #FB90
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$26,$0,0		% -32769 -> #FBA0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$27,$0,0		% -2147483648 -> #FBB0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STO		$28,$0,0		% -2147483649 -> #FBC0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16


		% test STOU(I)
		STOU		$2,$0,0		% 0 -> #FBD0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$3,$0,0		% 1 -> #FBE0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$4,$0,0		% 10 -> #FBF0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$5,$0,0		% 255 -> #FC00
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$6,$0,0		% 65535 -> #FC10
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$7,$0,0		% 4294967295 -> #FC20
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$8,$0,0		% -1 -> #FC30
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$9,$0,0		% 127 -> #FC40
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$10,$0,0		% 32767 -> #FC50
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$11,$0,0		% 2147483647 -> #FC60
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$12,$0,0		% 9223372036854775807 -> #FC70
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$13,$0,0		% 128 -> #FC80
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$14,$0,0		% 32768 -> #FC90
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$15,$0,0		% 2147483648 -> #FCA0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$16,$0,0		% -9223372036854775808 -> #FCB0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$17,$0,0		% 127 -> #FCC0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$18,$0,0		% 128 -> #FCD0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$19,$0,0		% 32767 -> #FCE0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$20,$0,0		% 32768 -> #FCF0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$21,$0,0		% 2147483647 -> #FD00
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$22,$0,0		% 2147483648 -> #FD10
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$23,$0,0		% -128 -> #FD20
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$24,$0,0		% -129 -> #FD30
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$25,$0,0		% -32768 -> #FD40
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$26,$0,0		% -32769 -> #FD50
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$27,$0,0		% -2147483648 -> #FD60
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STOU		$28,$0,0		% -2147483649 -> #FD70
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16


		% test STHT(I)
		STHT		$2,$0,0		% 0 -> #FD80
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$3,$0,0		% 1 -> #FD90
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$4,$0,0		% 10 -> #FDA0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$5,$0,0		% 255 -> #FDB0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$6,$0,0		% 65535 -> #FDC0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$7,$0,0		% 4294967295 -> #FDD0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$8,$0,0		% -1 -> #FDE0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$9,$0,0		% 127 -> #FDF0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$10,$0,0		% 32767 -> #FE00
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$11,$0,0		% 2147483647 -> #FE10
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$12,$0,0		% 9223372036854775807 -> #FE20
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$13,$0,0		% 128 -> #FE30
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$14,$0,0		% 32768 -> #FE40
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$15,$0,0		% 2147483648 -> #FE50
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$16,$0,0		% -9223372036854775808 -> #FE60
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$17,$0,0		% 127 -> #FE70
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$18,$0,0		% 128 -> #FE80
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$19,$0,0		% 32767 -> #FE90
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$20,$0,0		% 32768 -> #FEA0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$21,$0,0		% 2147483647 -> #FEB0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$22,$0,0		% 2147483648 -> #FEC0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$23,$0,0		% -128 -> #FED0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$24,$0,0		% -129 -> #FEE0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$25,$0,0		% -32768 -> #FEF0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$26,$0,0		% -32769 -> #FF00
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$27,$0,0		% -2147483648 -> #FF10
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STHT		$28,$0,0		% -2147483649 -> #FF20
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16


		% test STCO(I)
		STCO		#00,$0,0		% -> #FF30
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#01,$0,0		% -> #FF40
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#0A,$0,0		% -> #FF50
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#FF,$0,0		% -> #FF60
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#FF,$0,0		% -> #FF70
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#FF,$0,0		% -> #FF80
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#FF,$0,0		% -> #FF90
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#7F,$0,0		% -> #FFA0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#FF,$0,0		% -> #FFB0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#FF,$0,0		% -> #FFC0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#FF,$0,0		% -> #FFD0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#80,$0,0		% -> #FFE0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#00,$0,0		% -> #FFF0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#00,$0,0		% -> #10000
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#00,$0,0		% -> #10010
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#7F,$0,0		% -> #10020
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#80,$0,0		% -> #10030
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#FF,$0,0		% -> #10040
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#00,$0,0		% -> #10050
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#FF,$0,0		% -> #10060
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#00,$0,0		% -> #10070
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#80,$0,0		% -> #10080
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#7F,$0,0		% -> #10090
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#00,$0,0		% -> #100A0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#FF,$0,0		% -> #100B0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#00,$0,0		% -> #100C0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16

		STCO		#FF,$0,0		% -> #100D0
		GET		$1,rA
		PUT		rA,0
		STOU	$1,$0,8
		ADDU	$0,$0,16


		% Sync memory
		SETL	$1,#F000
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#FE,$1
		ADDU	$1,$1,#FF
		SYNCD	#2,$1
		ADDU	$1,$1,#3
		TRAP	0
