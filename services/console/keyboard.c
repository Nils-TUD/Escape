/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include "keyboard.h"
#include "set1.h"

#define BUFFER_SIZE		64
#define NOT_PRINTABLE	'?'

static u8 keycode2ascii[][128] = {
	/* default */
	{
		/* - none - */		NOT_PRINTABLE,
		/* VK_ACCENT */		'`',
		/* VK_0 */			'0',
		/* VK_1 */			'1',
		/* VK_2 */			'2',
		/* VK_3 */			'3',
		/* VK_4 */			'4',
		/* VK_5 */			'5',
		/* VK_6 */			'6',
		/* VK_7 */			'7',
		/* VK_8 */			'8',
		/* VK_9 */			'9',
		/* VK_MINUS */		'-',
		/* VK_EQ */			'=',
		/* --- */			NOT_PRINTABLE,
		/* VK_BACKSP */		NOT_PRINTABLE,
		/* VK_TAB */		'\t',
		/* VK_Q */			'q',
		/* VK_W */			'w',
		/* VK_E */			'e',
		/* VK_R */			'r',
		/* VK_T */			't',
		/* VK_Y */			'y',
		/* VK_U */			'u',
		/* VK_I */			'i',
		/* VK_O */			'o',
		/* VK_P */			'p',
		/* VK_LBRACKET */	'[',
		/* VK_RBRACKET */	']',
		/* VK_BACKSLASH */	'\\',
		/* VK_CAPS */		NOT_PRINTABLE,
		/* VK_A */			'a',
		/* VK_S */			's',
		/* VK_D */			'd',
		/* VK_F */			'f',
		/* VK_G */			'g',
		/* VK_H */			'h',
		/* VK_J */			'j',
		/* VK_K */			'k',
		/* VK_L */			'l',
		/* VK_SEM */		';',
		/* VK_APOS */		'\'',
		/* non-US-1 */		NOT_PRINTABLE,
		/* VK_ENTER */		'\n',
		/* VK_LSHIFT */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* VK_Z */			'z',
		/* VK_X */			'x',
		/* VK_C */			'c',
		/* VK_V */			'v',
		/* VK_B */			'b',
		/* VK_N */			'n',
		/* VK_M */			'm',
		/* VK_COMMA */		',',
		/* VK_DOT */		'.',
		/* VK_SLASH */		'/',
		/* --- */			NOT_PRINTABLE,
		/* VK_RSHIFT */		NOT_PRINTABLE,
		/* VK_LCTRL */		NOT_PRINTABLE,
		/* VK_LSUPER */		NOT_PRINTABLE,
		/* VK_LALT */		NOT_PRINTABLE,
		/* VK_SPACE */		' ',
		/* VK_RALT */		NOT_PRINTABLE,
		/* VK_APPS */		NOT_PRINTABLE,
		/* VK_RCTRL */		NOT_PRINTABLE,
		/* VK_RSUPER */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* VK_INSERT */		NOT_PRINTABLE,
		/* VK_DELETE */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* VK_LEFT */		NOT_PRINTABLE,
		/* VK_HOME */		NOT_PRINTABLE,
		/* VK_END */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* VK_UP */			NOT_PRINTABLE,
		/* VK_DOWN */		NOT_PRINTABLE,
		/* VK_PGUP */		NOT_PRINTABLE,
		/* VK_PGDOWN */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* VK_RIGHT */		NOT_PRINTABLE,
		/* VK_NUM */		NOT_PRINTABLE,
		/* VK_KP7 */		'7',
		/* VK_KP4 */		'4',
		/* VK_KP1 */		'1',
		/* --- */			NOT_PRINTABLE,
		/* VK_KPDIV */		'/',
		/* VK_KP8 */		'8',
		/* VK_KP5 */		'5',
		/* VK_KP2 */		'2',
		/* VK_KP0 */		'0',
		/* VK_KPMUL */		'*',
		/* VK_KP9 */		'9',
		/* VK_KP6 */		'6',
		/* VK_KP3 */		'3',
		/* VK_KPDOT */		'.',
		/* VK_KPSUB */		'-',
		/* VK_KPADD */		'+',
		/* --- */			NOT_PRINTABLE,
		/* VK_KPENTER */	'\n',
		/* --- */			NOT_PRINTABLE,
		/* VK_ESC */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* VK_F1 */			NOT_PRINTABLE,
		/* VK_F2 */			NOT_PRINTABLE,
		/* VK_F3 */			NOT_PRINTABLE,
		/* VK_F4 */			NOT_PRINTABLE,
		/* VK_F5 */			NOT_PRINTABLE,
		/* VK_F6 */			NOT_PRINTABLE,
		/* VK_F7 */			NOT_PRINTABLE,
		/* VK_F8 */			NOT_PRINTABLE,
		/* VK_F9 */			NOT_PRINTABLE,
		/* VK_F10 */		NOT_PRINTABLE,
		/* VK_F11 */		NOT_PRINTABLE,
		/* VK_F12 */		NOT_PRINTABLE,
		/* VK_PRINT */		NOT_PRINTABLE,
		/* VK_SCROLL */		NOT_PRINTABLE,
		/* VK_PAUSE */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
	},

	/* shift pressed */
	{
		/* - none - */		NOT_PRINTABLE,
		/* VK_ACCENT */		'~',
		/* VK_0 */			')',
		/* VK_1 */			'!',
		/* VK_2 */			'@',
		/* VK_3 */			'#',
		/* VK_4 */			'$',
		/* VK_5 */			'%',
		/* VK_6 */			'^',
		/* VK_7 */			'&',
		/* VK_8 */			'*',
		/* VK_9 */			'(',
		/* VK_MINUS */		'_',
		/* VK_EQ */			'+',
		/* --- */			NOT_PRINTABLE,
		/* VK_BACKSP */		NOT_PRINTABLE,
		/* VK_TAB */		'\t',
		/* VK_Q */			'Q',
		/* VK_W */			'W',
		/* VK_E */			'E',
		/* VK_R */			'R',
		/* VK_T */			'T',
		/* VK_Y */			'Y',
		/* VK_U */			'U',
		/* VK_I */			'I',
		/* VK_O */			'O',
		/* VK_P */			'P',
		/* VK_LBRACKET */	'{',
		/* VK_RBRACKET */	'}',
		/* VK_BACKSLASH */	'|',
		/* VK_CAPS */		NOT_PRINTABLE,
		/* VK_A */			'A',
		/* VK_S */			'S',
		/* VK_D */			'D',
		/* VK_F */			'F',
		/* VK_G */			'G',
		/* VK_H */			'H',
		/* VK_J */			'J',
		/* VK_K */			'K',
		/* VK_L */			'L',
		/* VK_SEM */		':',
		/* VK_APOS */		'"',
		/* non-US-1 */		NOT_PRINTABLE,
		/* VK_ENTER */		'\n',
		/* VK_LSHIFT */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* VK_Z */			'Z',
		/* VK_X */			'X',
		/* VK_C */			'C',
		/* VK_V */			'V',
		/* VK_B */			'B',
		/* VK_N */			'N',
		/* VK_M */			'M',
		/* VK_COMMA */		'<',
		/* VK_DOT */		'>',
		/* VK_SLASH */		'?',
		/* --- */			NOT_PRINTABLE,
		/* VK_RSHIFT */		NOT_PRINTABLE,
		/* VK_LCTRL */		NOT_PRINTABLE,
		/* VK_LSUPER */		NOT_PRINTABLE,
		/* VK_LALT */		NOT_PRINTABLE,
		/* VK_SPACE */		' ',
		/* VK_RALT */		NOT_PRINTABLE,
		/* VK_APPS */		NOT_PRINTABLE,
		/* VK_RCTRL */		NOT_PRINTABLE,
		/* VK_RSUPER */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* VK_INSERT */		NOT_PRINTABLE,
		/* VK_DELETE */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* VK_LEFT */		NOT_PRINTABLE,
		/* VK_HOME */		NOT_PRINTABLE,
		/* VK_END */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* VK_UP */			NOT_PRINTABLE,
		/* VK_DOWN */		NOT_PRINTABLE,
		/* VK_PGUP */		NOT_PRINTABLE,
		/* VK_PGDOWN */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* VK_RIGHT */		NOT_PRINTABLE,
		/* VK_NUM */		NOT_PRINTABLE,
		/* VK_KP7 */		'7',
		/* VK_KP4 */		'4',
		/* VK_KP1 */		'1',
		/* --- */			NOT_PRINTABLE,
		/* VK_KPDIV */		'/',
		/* VK_KP8 */		'8',
		/* VK_KP5 */		'5',
		/* VK_KP2 */		'2',
		/* VK_KP0 */		'0',
		/* VK_KPMUL */		'*',
		/* VK_KP9 */		'9',
		/* VK_KP6 */		'6',
		/* VK_KP3 */		'3',
		/* VK_KPDOT */		'.',
		/* VK_KPSUB */		'-',
		/* VK_KPADD */		'+',
		/* --- */			NOT_PRINTABLE,
		/* VK_KPENTER */	'\n',
		/* --- */			NOT_PRINTABLE,
		/* VK_ESC */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
		/* VK_F1 */			NOT_PRINTABLE,
		/* VK_F2 */			NOT_PRINTABLE,
		/* VK_F3 */			NOT_PRINTABLE,
		/* VK_F4 */			NOT_PRINTABLE,
		/* VK_F5 */			NOT_PRINTABLE,
		/* VK_F6 */			NOT_PRINTABLE,
		/* VK_F7 */			NOT_PRINTABLE,
		/* VK_F8 */			NOT_PRINTABLE,
		/* VK_F9 */			NOT_PRINTABLE,
		/* VK_F10 */		NOT_PRINTABLE,
		/* VK_F11 */		NOT_PRINTABLE,
		/* VK_F12 */		NOT_PRINTABLE,
		/* VK_PRINT */		NOT_PRINTABLE,
		/* VK_SCROLL */		NOT_PRINTABLE,
		/* VK_PAUSE */		NOT_PRINTABLE,
		/* --- */			NOT_PRINTABLE,
	},
};

static bool shift = false;
static bool alt = false;
static bool ctrl = false;
static u8 pos = 0;
static u8 buffer[BUFFER_SIZE];

void kb_handleScanCode(u8 scanCode) {
	u8 keycode = kb_set1_getKeycode(scanCode);
	if(keycode) {
		s8 c;
		switch(keycode) {
			case VK_LSHIFT:
			case VK_RSHIFT:
				break;
		}
		if(keycode2ascii[0][keycode] != NOT_PRINTABLE) {
			vid_putchar(keycode2ascii[0][keycode]);
		}
		else
			vid_printf("\nkeycode=%d\n",keycode);

		/* buffer full? */
		if(pos >= BUFFER_SIZE)
			return;

		buffer[pos++] = keycode;
	}
}

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
