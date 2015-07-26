%
% fint-manual.mms -- tests the algo proposed in mmix-doc to calculate FINT
%
		LOC		#1000

		% Put floats in registers
Main	SET		$21,#0000		% 0.0
		PUSHJ	$20,mkint
		SET		$0,$20

		SETH	$21,#8000		% -0.0
		PUSHJ	$20,mkint
		SET		$1,$20

		SETH	$21,#3FF0		% 1.0
		PUSHJ	$20,mkint
		SET		$2,$20

		SETH	$21,#4000		% 2.0
		PUSHJ	$20,mkint
		SET		$3,$20

		SETH	$21,#BFF8		% -1.5
		PUSHJ	$20,mkint
		SET		$4,$20

		SETH	$21,#C029
		ORMH	$21,#84C5
		ORML	$21,#974E
		ORL		$21,#65BF		% -12.75932
		PUSHJ	$20,mkint
		SET		$5,$20

		SETH	$21,#4088
		ORMH	$21,#48EA
		ORML	$21,#6D26
		ORL		$21,#7408		% 777.114466
		PUSHJ	$20,mkint
		SET		$6,$20

		SETH	$21,#7FF0		% +inf
		PUSHJ	$20,mkint
		SET		$7,$20

		SETH	$21,#FFF0		% -inf
		PUSHJ	$20,mkint
		SET		$8,$20

		SETH	$21,#7FF8		% quiet NaN
		PUSHJ	$20,mkint
		SET		$9,$20

		SETH	$21,#7FF4		% signaling NaN
		PUSHJ	$20,mkint
		SET		$10,$20

		TRAP	0

% note that the trick is to add 2^52 to the number and substract it again. this way we cut the
% fraction-part because we cant represent a fraction when the number is >= 2^52.

% octa mkint(double d)
mkint	SETH	$1,#4330		% $1 = 2^52
		SET		$2,$0			% $2 = $Z
		ANDNH	$2,#8000		% $2 = abs($Z)
		ANDN	$3,$0,$2		% $3 = signbit($Z)
		FUN		$4,$0,$0		% $4 = [$Z is a NaN]
		BNZ		$4,1F			% skip ahead if $Z is a NaN
		FCMP	$4,$2,$1		% $4 = [abs($Z) > 2^52] - [abs($Z) < 2^52]
		CSNN	$1,$4,0			% set $1 = 0 if $4 >= 0
		OR		$1,$3,$1		% attach sign of $Z to $1
1H		FADD	$2,$0,$1		% $2 = $Z + $1
		FSUB	$0,$2,$1		% $X = $2 - $1
		% fix: FINT will never change the sign of $Z. therefore we simply set the orignal sign
		% again. this fixes the problem that occurs when integerizing -0.0.
		% the problem is that (-0.0 + X) - X is not -0.0 but +0.0.
		SETH	$2,#8000
		ANDN	$0,$0,$2		% $0 = $0 & ~sign($0)
		OR		$0,$0,$3		% $0 = $0 | sign($Z)
		POP		1,0
