%
% primegen.mms -- test knuths primes programm (slightly modified)
%

		% stack for unsave
		LOC		#200000
		OCTA	#0							% rL
		OCTA	#1000						% $244 = f4
		OCTA	#1							% $245 = f5
		OCTA	#1							% $246 = f6
		OCTA	#1							% $247 = f7
		OCTA	#1							% $248 = f8
		OCTA	#80000000001001F8			% $249 = f9
		OCTA	#8000000000100000			% $250 = fa
		OCTA	#1							% $251 = fb
		OCTA	#1							% $252 = fc
		OCTA	#1							% $253 = fd
		OCTA	#1							% $254 = fe
		OCTA	#2							% $255 = ff
		OCTA	#0							% rB
		OCTA	#0							% rD
		OCTA	#0							% rE
		OCTA	#0							% rH
		OCTA	#0							% rJ
		OCTA	#0							% rM
		OCTA	#0							% rP
		OCTA	#0							% rR
		OCTA	#0							% rW
		OCTA	#0							% rX
		OCTA	#0							% rY
		OCTA	#0							% rZ
ADDR	OCTA	#F400000000000000			% rG | rA

		% first prime
		LOC		#100000
		WYDE	2

		% last prime
		LOC		#1001F8
		WYDE	(1-252)*2
		
		LOC		#1000

		% setup our environment
Main	SETH	$0,#8000					% access it over the direct mapped space
		ORMH	$0,ADDR>>32					% otherwise we would have to setup paging
		ORML	$0,ADDR>>16
		ORL		$0,ADDR
		UNSAVE	$0
		JMP		Gen

DivOff	IS		$244
Offset	IS		$245
Prime	IS		$248
End		IS		$249
Primes	IS		$250

Gen		LDW		Offset,End,0				% load offset (negative)
		SETL	Prime,3						% second prime is 3
1H		STW		Prime,End,Offset			% store prime in current slot
		INCL	Offset,2					% walk to next slot (we are storing wydes)
		BZ		Offset,4F					% if offset is zero, we are done
2H		INCL	Prime,2						% calc next prime
		ADD		DivOff,Primes,2				% start with 3 as divisor
3H		LDW		$0,DivOff,0					% x = M[$244]
		DIV		$1,Prime,$0					% z = Prime / x
		GET		$2,rR
		BZ		$2,2B						% if Prime % x == 0, its no prime
		INCL	DivOff,2					% to next divisor
		PBNZ	$0,3B						% if x was not zero, continue
		JMP		1B							% we are done, its a prime, so store it
4H		SET		$0,0
		SYNCD	#FF,Primes,$0
		INCL	$0,#100
		SYNCD	#FF,Primes,$0
		
		TRAP	0
