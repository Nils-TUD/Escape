#
# c1.s -- end-of-segment labels
#

	.global	_ecode
	.global	_edata
	.global	_ebss
	.global stack

.section .text
	.align	4
_ecode:

.section .data
	# stack for unsave
	OCTA	#0								% rL
	OCTA	#0								% $255 = ff
	OCTA	#0								% rB
	OCTA	#0								% rD
	OCTA	#0								% rE
	OCTA	#0								% rH
	OCTA	#0								% rJ
	OCTA	#0								% rM
	OCTA	#0								% rP
	OCTA	#0								% rR
	OCTA	#0								% rW
	OCTA	#0								% rX
	OCTA	#0								% rY
	OCTA	#0								% rZ
stack:
	OCTA	#FF00000000000000	% rG | rA
	.align	4
_edata:

.section .bss
	.align	4
_ebss:
