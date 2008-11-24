/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVKEYBOARD_H_
#define PRIVKEYBOARD_H_

#include "../pub/common.h"
#include "../pub/keyboard.h"

#define KBD_DATA_PORT	0x60

#define	A_NPRINT		'^'

#define K_ERR			0x00
#define K_ESC			0x01
#define K_BACKSP		0x02
#define K_LCTRL			0x03
#define K_RCTRL			0x04
#define K_LALT			0x05
#define K_RALT			0x06
#define K_PRINT			0x07
#define K_LSHIFT		0x08
#define K_RSHIFT		0x09
#define K_CAPSLOCK		0x0A
#define K_F1			0x0B
#define K_F2			0x0C
#define K_F3			0x0D
#define K_F4			0x0E
#define K_F5			0x0F
#define K_F6			0x10
#define K_F7			0x11
#define K_F8			0x12
#define K_F9			0x13
#define K_F10			0x14
#define K_F11			0x15
#define K_F12			0x16
#define K_NUMTGL		0x17
#define K_SCROLL		0x18

/*
2b (\|), on a 102-key keyboard

2c (Z), 2d (X), 2e (C), 2f (V), 30 (B), 31 (N), 32 (M), 33 (,<), 34 (.>), 35 (/?), 36 (RShift)

37 (Keypad-*) or (* /PrtScn) on a 83/84-key keyboard

38 (LAlt), 39 (Space bar),

3a (CapsLock)

3b (F1), 3c (F2), 3d (F3), 3e (F4), 3f (F5), 40 (F6), 41 (F7), 42 (F8), 43 (F9), 44 (F10)

45 (NumLock)

46 (ScrollLock)

47 (Keypad-7/Home), 48 (Keypad-8/Up), 49 (Keypad-9/PgUp)

4a (Keypad--)

4b (Keypad-4/Left), 4c (Keypad-5), 4d (Keypad-6/Right), 4e (Keypad-+)

4f (Keypad-1/End), 50 (Keypad-2/Down), 51 (Keypad-3/PgDn)

52 (Keypad-0/Ins), 53 (Keypad-./Del)

54 (Alt-SysRq) on a 84+ key keyboard */

#define SCAN_LSHIFT				0x2A
#define SCAN_RSHIFT				0x36
#define SCAN_SEQ_INIT			0xE0
#define SCAN_SEQ_NOBR_INIT		0xE1
#define IS_BREAK(code) (code & 0x80)

#define KBD_BUF_SIZE			10

#endif /* PRIVKEYBOARD_H_ */
