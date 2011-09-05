%
% go.mms -- tests the go-instruction
%

		LOC		#1000

Main	GETA	$0,F1
		SET		$1,0
		SET		$2,0
		GO		$0,$0,0
		SET		$2,2
		
		% test not tetra-aligned PCs
		GETA	$0,@+12
		ADDU	$0,$0,1
		GO		$0,$0,0
		SET		$3,3
		SET		$4,4
		GETA	$5,@
		
		TRAP	0

F1		SET		$1,1
		GO		$0,$0,0
