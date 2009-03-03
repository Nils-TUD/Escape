/**
 * @version		$Id: video.h 77 2008-11-22 22:27:35Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef VIDEO_H_
#define VIDEO_H_

#if IN_KERNEL
#	include "../../kernel/h/common.h"
#else
#	include "../../libc/h/common.h"
#endif
#include <stdarg.h>

typedef enum {BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE} eColor;

/**
 * Inits the video-stuff
 */
void vid_init(void);

/**
 * Clears the screen
 */
void vid_clearScreen(void);

/**
 * Sets the background-color of the given line to the value
 *
 * @param line the line (0..n-1)
 * @param bg the background-color
 */
void vid_setLineBG(u8 line,eColor bg);

/**
 * Restores the color that has been saved by vid_useColor.
 */
void vid_restoreColor(void);

/**
 * @return the current foreground color
 */
eColor vid_getFGColor(void);

/**
 * @return the current background color
 */
eColor vid_getBGColor(void);

/**
 * Sets the foreground color to given value
 *
 * @param col the new color
 */
void vid_setFGColor(eColor col);

/**
 * Sets the background color to given value
 *
 * @param col the new color
 */
void vid_setBGColor(eColor col);

/**
 * Sets the given color and stores the current one. You may restore the previous state
 * by calling vid_restoreColor().
 *
 * @param bg your background-color
 * @param fg your foreground-color
 */
void vid_useColor(eColor bg,eColor fg);

/**
 * @return the line-number
 */
u8 vid_getLine(void);

/**
 * Walks to (lineEnd - pad), so that for example a string can be printed at the end of the line.
 * Note that this may overwrite existing characters!
 *
 * @param pad the number of characters to reach the line-end
 */
void vid_toLineEnd(u8 pad);

/**
 * Prints the given character to the current position on the screen
 *
 * @param c the character
 */
void vid_putchar(s8 c);

/**
 * Prints the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 */
void vid_printu(u32 n,u8 base);

/**
 * Determines the width of the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 * @return the width
 */
u8 vid_getuwidth(u32 n,u8 base);

/**
 * Prints the given string on the screen
 *
 * @param str the string
 */
void vid_puts(cstring str);

/**
 * Determines the width of the given string
 *
 * @param str the string
 * @return the width
 */
u8 vid_getswidth(cstring str);

/**
 * Prints the given signed 32-bit integer in base 10
 *
 * @param n the integer
 */
void vid_printn(s32 n);

/**
 * Determines the width of the given signed 32-bit integer in base 10
 *
 * @param n the integer
 * @return the width
 */
u8 vid_getnwidth(s32 n);

/**
 * The kernel-version of printf. Currently it supports:
 * %d: signed integer
 * %u: unsigned integer, base 10
 * %o: unsigned integer, base 8
 * %x: unsigned integer, base 16 (small letters)
 * %X: unsigned integer, base 16 (big letters)
 * %b: unsigned integer, base 2
 * %s: string
 * %c: character
 *
 * @param fmt the format
 */
void vid_printf(cstring fmt,...);

/**
 * Same as vid_printf, but with the va_list as argument
 *
 * @param fmt the format
 * @param ap the argument-list
 */
void vid_vprintf(cstring fmt,va_list ap);

#endif
