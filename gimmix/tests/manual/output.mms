%
% output.mms -- write to output-file
%

		LOC		#0
STR		BYTE	"Hello World output-file!",#d,#a,0

		LOC		#1000

		% write welcome-message
Main	SET		$1,STR
		ORH		$1,#8000
		PUSHJ	$0,io:oputs
		
		JMP		Main

#include "io.mmi"
