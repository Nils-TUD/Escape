%
% put.mms -- tests a special case of PUT
%

		LOC		#1000

Main	PUT		rQ,#1
		GET		$1,rQ
		PUT		rQ,#0
		GET		$2,rQ
		PUT		rQ,#1
		PUT		rQ,#2	% cant unset #1
		GET		$3,rQ	% -> #3

		TRAP	0
