//
// c1.s -- end-of-segment labels
//

.section .data
	// stack for unsave
	OCTA	#0								// rL
	OCTA	#0								// $255 = ff
	OCTA	#0								// rB
	OCTA	#0								// rD
	OCTA	#0								// rE
	OCTA	#0								// rH
	OCTA	#0								// rJ
	OCTA	#0								// rM
	OCTA	#0								// rP
	OCTA	#0								// rR
	OCTA	#0								// rW
	OCTA	#0								// rX
	OCTA	#0								// rY
	OCTA	#0								// rZ
stack:
	OCTA	#FF00000000000000				// rG | rA
	.align	8

.section .bss
	.align	8

// This must be the last file on the link-line, allocating global registers
// from the top.

// Register $254 is the stack-pointer.
sp GREG

// Register $253 is frame-pointer.  It's not supposed to be used in most
// functions.
fp GREG

// $252 is the static chain register; nested functions receive the
// context of the surrounding function through a pointer passed in this
// register.
static_chain GREG
struct_value_reg GREG

// These registers are used to pass state at an exceptional return (C++).
eh_state_3 GREG
eh_state_2 GREG
eh_state_1 GREG
eh_state_0 GREG

#if defined(__MMIX_ABI_GNU__)

// Allocate global registers used by the GNU ABI.
gnu_parm_reg_16 GREG
gnu_parm_reg_15 GREG
gnu_parm_reg_14 GREG
gnu_parm_reg_13 GREG
gnu_parm_reg_12 GREG
gnu_parm_reg_11 GREG
gnu_parm_reg_10 GREG
gnu_parm_reg_9 GREG
gnu_parm_reg_8 GREG
gnu_parm_reg_7 GREG
gnu_parm_reg_6 GREG
gnu_parm_reg_5 GREG
gnu_parm_reg_4 GREG
gnu_parm_reg_3 GREG
gnu_parm_reg_2 GREG
gnu_parm_reg_1 GREG

#endif /* __MMIX_ABI_GNU__ */
