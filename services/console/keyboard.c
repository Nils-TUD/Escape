/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <keycodes.h>

#define BUFFER_SIZE		64
#define NPRINT			'?'

typedef struct {
	s8 def;
	s8 shift;
	s8 alt;
} sKeymapEntry;

static sKeymapEntry keymap[] = {
	/* - none - */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_ACCENT */		{'`',		'~',		NPRINT},
	/* VK_0 */			{'0',		')',		NPRINT},
	/* VK_1 */			{'1',		'!',		NPRINT},
	/* VK_2 */			{'2',		'@',		NPRINT},
	/* VK_3 */			{'3',		'#',		NPRINT},
	/* VK_4 */			{'4',		'$',		NPRINT},
	/* VK_5 */			{'5',		'%',		NPRINT},
	/* VK_6 */			{'6',		'^',		NPRINT},
	/* VK_7 */			{'7',		'&',		NPRINT},
	/* VK_8 */			{'8',		'*',		NPRINT},
	/* VK_9 */			{'9',		'(',		NPRINT},
	/* VK_MINUS */		{'-',		'_',		NPRINT},
	/* VK_EQ */			{'=',		'+',		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_BACKSP */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_TAB */		{'\t',		'\t',		NPRINT},
	/* VK_Q */			{'q',		'Q',		NPRINT},
	/* VK_W */			{'w',		'W',		NPRINT},
	/* VK_E */			{'e',		'E',		NPRINT},
	/* VK_R */			{'r',		'R',		NPRINT},
	/* VK_T */			{'t',		'T',		NPRINT},
	/* VK_Y */			{'y',		'Y',		NPRINT},
	/* VK_U */			{'u',		'U',		NPRINT},
	/* VK_I */			{'i',		'I',		NPRINT},
	/* VK_O */			{'o',		'O',		NPRINT},
	/* VK_P */			{'p',		'P',		NPRINT},
	/* VK_LBRACKET */	{'[',		'{',		NPRINT},
	/* VK_RBRACKET */	{']',		'}',		NPRINT},
	/* VK_BACKSLASH */	{'\\',		'|',		NPRINT},
	/* VK_CAPS */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_A */			{'a',		'A',		NPRINT},
	/* VK_S */			{'s',		'S',		NPRINT},
	/* VK_D */			{'d',		'D',		NPRINT},
	/* VK_F */			{'f',		'F',		NPRINT},
	/* VK_G */			{'g',		'G',		NPRINT},
	/* VK_H */			{'h',		'H',		NPRINT},
	/* VK_J */			{'j',		'J',		NPRINT},
	/* VK_K */			{'k',		'K',		NPRINT},
	/* VK_L */			{'l',		'L',		NPRINT},
	/* VK_SEM */		{';',		':',		NPRINT},
	/* VK_APOS */		{'\'',		'"',		NPRINT},
	/* non-US-1 */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_ENTER */		{'\n',		'\n',		NPRINT},
	/* VK_LSHIFT */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_Z */			{'z',		'Z',		NPRINT},
	/* VK_X */			{'x',		'X',		NPRINT},
	/* VK_C */			{'c',		'C',		NPRINT},
	/* VK_V */			{'v',		'V',		NPRINT},
	/* VK_B */			{'b',		'B',		NPRINT},
	/* VK_N */			{'n',		'N',		NPRINT},
	/* VK_M */			{'m',		'M',		NPRINT},
	/* VK_COMMA */		{',',		'<',		NPRINT},
	/* VK_DOT */		{'.',		'>',		NPRINT},
	/* VK_SLASH */		{'/',		'?',		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_RSHIFT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_LCTRL */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_LSUPER */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_LALT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_SPACE */		{' ',		' ',		NPRINT},
	/* VK_RALT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_APPS */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_RCTRL */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_RSUPER */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_INSERT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_DELETE */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_LEFT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_HOME */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_END */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_UP */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_DOWN */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_PGUP */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_PGDOWN */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_RIGHT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_NUM */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_KP7 */		{'7',		'7',		NPRINT},
	/* VK_KP4 */		{'4',		'4',		NPRINT},
	/* VK_KP1 */		{'1',		'1',		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_KPDIV */		{'/',		'/',		NPRINT},
	/* VK_KP8 */		{'8',		'8',		NPRINT},
	/* VK_KP5 */		{'5',		'5',		NPRINT},
	/* VK_KP2 */		{'2',		'2',		NPRINT},
	/* VK_KP0 */		{'0',		'0',		NPRINT},
	/* VK_KPMUL */		{'*',		'*',		NPRINT},
	/* VK_KP9 */		{'9',		'9',		NPRINT},
	/* VK_KP6 */		{'6',		'6',		NPRINT},
	/* VK_KP3 */		{'3',		'3',		NPRINT},
	/* VK_KPDOT */		{'.',		'.',		NPRINT},
	/* VK_KPSUB */		{'-',		'-',		NPRINT},
	/* VK_KPADD */		{'+',		'+',		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_KPENTER */	{'\n',		'\n',		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_ESC */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F1 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F2 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F3 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F4 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F5 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F6 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F7 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F8 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F9 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F10 */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_F11 */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_F12 */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_PRINT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_SCROLL */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_PAUSE */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
};

static bool shift = false;
static bool alt = false;
static bool ctrl = false;
static u8 pos = 0;
static u8 buffer[BUFFER_SIZE];

void kb_handleScanCode(u8 scanCode) {
	bool brk;
	u8 keycode = 0;/*kb_set1_getKeycode(scanCode,&brk);*/
	if(keycode) {
		sKeymapEntry *e;
		s8 c;
		switch(keycode) {
			case VK_LSHIFT:
			case VK_RSHIFT:
				shift = !brk;
				break;
			case VK_LALT:
			case VK_RALT:
				alt = !brk;
				break;
			case VK_LCTRL:
			case VK_RCTRL:
				ctrl = !brk;
				break;
		}

		/* don't print break-keycodes */
		if(brk)
			return;

		e = keymap + keycode;
		if(shift)
			c = e->shift;
		else if(alt)
			c = e->alt;
		else
			c = e->def;

		/*if(c != NPRINT)
			vid_putchar(c);*/
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
