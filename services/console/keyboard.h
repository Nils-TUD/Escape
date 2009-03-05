/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <common.h>

/* virtual key-codes */
/* all printable characters are encoded as ASCII-code */
/* all other are encoded with 0x80 + X */
/*#define VK_A			'a'
#define VK_B			'b'
#define VK_C			'c'
#define VK_D			'd'
#define VK_E			'e'
#define VK_F			'f'
#define VK_G			'g'
#define VK_H			'h'
#define VK_I			'i'
#define VK_J			'j'
#define VK_K			'k'
#define VK_L			'l'
#define VK_M			'm'
#define VK_N			'n'
#define VK_O			'o'
#define VK_P			'p'
#define VK_Q			'q'
#define VK_R			'r'
#define VK_S			's'
#define VK_T			't'
#define VK_U			'u'
#define VK_V			'v'
#define VK_W			'w'
#define VK_X			'x'
#define VK_Y			'y'
#define VK_Z			'z'
#define VK_0			'0'
#define VK_1			'1'
#define VK_2			'2'
#define VK_3			'3'
#define VK_4			'4'
#define VK_5			'5'
#define VK_6			'6'
#define VK_7			'7'
#define VK_8			'8'
#define VK_9			'9'
#define VK_ACCENT		'`'
#define VK_MINUS		'-'
#define VK_EQ			'='
#define VK_BACKSLASH	'\\'
#define VK_BACKSP		'\b'
#define VK_SPACE		' '
#define VK_TAB			'\t'
#define VK_CAPS			0x80
#define VK_LSHIFT		0x81
#define VK_LCTRL		0x82
#define VK_LSUPER		0x83
#define VK_LALT			0x84
#define VK_RSHIFT		0x85
#define VK_RCTRL		0x86
#define VK_RSUPER		0x87
#define VK_RALT			0x88
#define VK_APPS			0x89
#define VK_ENTER		'\n'
#define VK_ESC			0x1B
#define VK_F1			0x8A
#define VK_F2			0x8B
#define VK_F3			0x8C
#define VK_F4			0x8D
#define VK_F5			0x8E
#define VK_F6			0x8F
#define VK_F7			0x90
#define VK_F8			0x91
#define VK_F9			0x92
#define VK_F10			0x93
#define VK_F11			0x94
#define VK_F12			0x95
#define VK_PRINT		0x96
#define VK_SCROLL		0x97
#define VK_PAUSE		0x98
#define VK_LBRACKET		'['
#define VK_INSERT		0x99
#define VK_HOME			0x9A
#define VK_PGUP			0x9B
#define VK_DELETE		0x9C
#define VK_END			0x9D
#define VK_PGDOWN		0x9E
#define VK_UP			0x9F
#define VK_LEFT			0xA0
#define VK_DOWN			0xA1
#define VK_RIGHT		0xA2
#define VK_NUM			0xA3
#define VK_KPDIV		0xA4
#define VK_KPMUL		0xA5
#define VK_KPSUB		0xA6
#define VK_KPADD		0xA7
#define VK_KPEN			0xA8
#define VK_KPDOT		0xA9
#define VK_KP0			0xAA
#define VK_KP1			0xAB
#define VK_KP2			0xAC
#define VK_KP3			0xAD
#define VK_KP4			0xAE
#define VK_KP5			0xAF
#define VK_KP6			0xB0
#define VK_KP7			0xB1
#define VK_KP8			0xB2
#define VK_KP9			0xB3
#define VK_RBRACKET		']'
#define VK_SEM			';'
#define VK_APOS			'\''
#define VK_COMMA		','
#define VK_DOT			'.'
#define VK_SLASH		'/'*/

#define VK_ACCENT		1
#define VK_0			2
#define VK_1			3
#define VK_2			4
#define VK_3			5
#define VK_4			6
#define VK_5			7
#define VK_6			8
#define VK_7			9
#define VK_8			10
#define VK_9			11
#define VK_MINUS		12
#define VK_EQ			13
#define VK_BACKSP		15
#define VK_TAB			16
#define VK_Q			17
#define VK_W			18
#define VK_E			19
#define VK_R			20
#define VK_T			21
#define VK_Y			22
#define VK_U			23
#define VK_I			24
#define VK_O			25
#define VK_P			26
#define VK_LBRACKET		27
#define VK_RBRACKET		28
#define VK_BACKSLASH	29
#define VK_CAPS			30
#define VK_A			31
#define VK_S			32
#define VK_D			33
#define VK_F			34
#define VK_G			35
#define VK_H			36
#define VK_J			37
#define VK_K			38
#define VK_L			39
#define VK_SEM			40
#define VK_APOS			41
/* non-US-1 ?? */
#define VK_ENTER		43
#define VK_LSHIFT		44
#define VK_Z			46
#define VK_X			47
#define VK_C			48
#define VK_V			49
#define VK_B			50
#define VK_N			51
#define VK_M			52
#define VK_COMMA		53
#define VK_DOT			54
#define VK_SLASH		55
#define VK_RSHIFT		57
#define VK_LCTRL		58
#define VK_LSUPER		59
#define VK_LALT			60
#define VK_SPACE		61
#define VK_RALT			62
#define VK_APPS			63	/* ?? */
#define VK_RCTRL		64
#define VK_RSUPER		65
#define VK_INSERT		75
#define VK_DELETE		76
#define VK_HOME			80
#define VK_END			81
#define VK_PGUP			85
#define VK_PGDOWN		86
#define VK_LEFT			79
#define VK_UP			83
#define VK_DOWN			84
#define VK_RIGHT		89
#define VK_NUM			90
#define VK_KP7			91
#define VK_KP4			92
#define VK_KP1			93
#define VK_KPDIV		95
#define VK_KP8			96
#define VK_KP5			97
#define VK_KP2			98
#define VK_KP0			99
#define VK_KPMUL		100
#define VK_KP9			101
#define VK_KP6			102
#define VK_KP3			103
#define VK_KPDOT		104
#define VK_KPSUB		105
#define VK_KPADD		106
#define VK_KPENTER		108
#define VK_ESC			110
#define VK_F1			112
#define VK_F2			113
#define VK_F3			114
#define VK_F4			115
#define VK_F5			116
#define VK_F6			117
#define VK_F7			118
#define VK_F8			119
#define VK_F9			120
#define VK_F10			121
#define VK_F11			122
#define VK_F12			123
#define VK_PRINT		124
#define VK_SCROLL		125
#define VK_PAUSE		126



/*
KEY  	 MAKE  	 BREAK  	 -----  	 KEY  	 MAKE  	 BREAK  	 -----  	 KEY  	 MAKE  	 BREAK
A 	1E 	9E 	  	9 	0A 	8A 	  	[ 	1A 	9A
B 	30 	B0 	  	` 	29 	89 	  	INSERT 	E0,52 	E0,D2
C 	2E 	AE 	  	- 	0C 	8C 	  	HOME 	E0,47 	E0,97
D 	20 	A0 	  	= 	0D 	8D 	  	PG UP 	E0,49 	E0,C9
E 	12 	92 	  	\ 	2B 	AB 	  	DELETE 	E0,53 	E0,D3
F 	21 	A1 	  	BKSP 	0E 	8E 	  	END 	E0,4F 	E0,CF
G 	22 	A2 	  	SPACE 	39 	B9 	  	PG DN 	E0,51 	E0,D1
H 	23 	A3 	  	TAB 	0F 	8F 	  	U ARROW 	E0,48 	E0,C8
I 	17 	97 	  	CAPS 	3A 	BA 	  	L ARROW 	E0,4B 	E0,CB
J 	24 	A4 	  	L SHFT 	2A 	AA 	  	D ARROW 	E0,50 	E0,D0
K 	25 	A5 	  	L CTRL 	1D 	9D 	  	R ARROW 	E0,4D 	E0,CD
L 	26 	A6 	  	L GUI 	E0,5B 	E0,DB 	  	NUM 	45 	C5
M 	32 	B2 	  	L ALT 	38 	B8 	  	KP / 	E0,35 	E0,B5
N 	31 	B1 	  	R SHFT 	36 	B6 	  	KP * 	37 	B7
O 	18 	98 	  	R CTRL 	E0,1D 	E0,9D 	  	KP - 	4A 	CA
P 	19 	99 	  	R GUI 	E0,5C 	E0,DC 	  	KP + 	4E 	CE
Q 	10 	19 	  	R ALT 	E0,38 	E0,B8 	  	KP EN 	E0,1C 	E0,9C
R 	13 	93 	  	APPS 	E0,5D 	E0,DD 	  	KP . 	53 	D3
S 	1F 	9F 	  	ENTER 	1C 	9C 	  	KP 0 	52 	D2
T 	14 	94 	  	ESC 	01 	81 	  	KP 1 	4F 	CF
U 	16 	96 	  	F1 	3B 	BB 	  	KP 2 	50 	D0
V 	2F 	AF 	  	F2 	3C 	BC 	  	KP 3 	51 	D1
W 	11 	91 	  	F3 	3D 	BD 	  	KP 4 	4B 	CB
X 	2D 	AD 	  	F4 	3E 	BE 	  	KP 5 	4C 	CC
Y 	15 	95 	  	F5 	3F 	BF 	  	KP 6 	4D 	CD
Z 	2C 	AC 	  	F6 	40 	C0 	  	KP 7 	47 	C7
0 	0B 	8B 	  	F7 	41 	C1 	  	KP 8 	48 	C8
1 	02 	82 	  	F8 	42 	C2 	  	KP 9 	49 	C9
2 	03 	83 	  	F9 	43 	C3 	  	] 	1B 	9B
3 	04 	84 	  	F10 	44 	C4 	  	; 	27 	A7
4 	05 	85 	  	F11 	57 	D7 	  	' 	28 	A8
5 	06 	86 	  	F12 	58 	D8 	  	, 	33 	B3
6 	07 	87 	  	PRNT
SCRN 	E0,2A,
E0,37  	 E0,B7,
E0,AA 	  	. 	34 	B4
7 	08 	88 	  	SCROLL 	46 	C6 	  	/ 	35 	B5

8
	09 	89 	  	PAUSE 	E1,1D,45
E1,9D,C5 	-NONE-
*/

#endif /* KEYBOARD_H_ */
