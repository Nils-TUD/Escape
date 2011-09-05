%
% jump-limits.mms -- tests the limits of jumps and branches
%
		LOC		#1000

Main	SET		$0,0

		% maximum possible forward-branch (#1004 -> #41000 = @ + 262140)
		BZ		$0,1F
		
2H		SET		$3,#44
		% maximum possible forward-jump (#100C -> #4001008 = @ + 67108860)
		JMP		3F
4H		SET		$4,#55
		TRAP	0

		LOC		#41000
1H		SET		$1,#22
		OR		$0,$0,$0
		% maximum possible backwards-branch (#41008 -> #1008 = @ - 262144)
		BZ		$0,2B

		LOC		#4001008
3H		SET		$2,#33
		OR		$0,$0,$0
		% maximum possible backwards-jump (#4001010 -> #1010 = @ - 67108864)
		JMP		4B
