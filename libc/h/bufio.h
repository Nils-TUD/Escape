/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef BUFIO_H_
#define BUFIO_H_

#include "common.h"
#include <stdarg.h>

/* TODO add constant EOF? */
/* TODO think about the return-values of print* */
/* TODO add sprintf and so on */

/**
 * Prints the given character to STDOUT
 *
 * @param c the character
 * @return the character or the error-code if failed
 */
s32 printc(char c);

/**
 * Prints the given character to <fd>
 *
 * @param fd the file-descriptor
 * @param c the character
 * @return the character or the error-code if failed
 */
s32 fprintc(tFD fd,char c);

/**
 * Prints the given string to STDOUT
 *
 * @param str the string
 * @return the number of written chars
 */
s32 prints(const char *str);

/**
 * Prints the given string to <fd>
 *
 * @param fd the file-descriptor
 * @param str the string
 * @return the number of written chars
 */
s32 fprints(tFD fd,const char *str);

/**
 * Prints the given signed integer to STDOUT
 *
 * @param n the number
 * @return the number of written chars
 */
s32 printn(s32 n);

/**
 * Prints the given signed integer to <fd>
 *
 * @param n the number
 * @return the number of written chars
 */
s32 fprintn(tFD fd,s32 n);

/**
 * Prints the given unsigned integer to the given base to STDOUT
 *
 * @param u the number
 * @param base the base (2 .. 16)
 * @return the number of written chars
 */
s32 printu(s32 n,u8 base);

/**
 * Prints the given unsigned integer to the given base to <fd>
 *
 * @param u the number
 * @param base the base (2 .. 16)
 * @return the number of written chars
 */
s32 fprintu(tFD fd,s32 u,u8 base);

/**
 * Formated output to STDOUT. Supports:
 * 	%d: signed integer
 * 	%u: unsigned integer, base 10
 * 	%o: unsigned integer, base 8
 * 	%x: unsigned integer, base 16 (small letters)
 * 	%X: unsigned integer, base 16 (big letters)
 * 	%b: unsigned integer, base 2
 * 	%s: string
 * 	%c: character
 * Additionally you can specify padding:
 * 	%02d
 * 	% 4x
 *  ...
 *
 * @param fmt the format
 * @return the number of written chars
 */
s32 printf(const char *fmt,...);

/**
 * Like printf(), but prints to <fd>
 *
 * @param fd the file-descriptor
 * @param fmt the format
 * @return the number of written chars
 */
s32 fprintf(tFD fd,const char *fmt,...);

/**
 * Like printf(), but lets you specify the argument-list
 *
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of written chars
 */
s32 vprintf(const char *fmt,va_list ap);

/**
 * Like vprintf(), but prints to <fd>
 *
 * @param fd the file-descriptor
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of written chars
 */
s32 vfprintf(tFD fd,const char *fmt,va_list ap);

/**
 * Flushes the output-channel of STDOUT
 */
void flush(void);

/**
 * Flushes the output-channel <fd>
 *
 * @param fd the file-descriptor
 */
void fflush(tFD fd);

/**
 * Reads one char from STDIN
 *
 * @return the character or 0
 */
char scanc(void);

/**
 * Reads one char from <fd>
 *
 * @param fd the file-descriptor
 * @return the character or 0
 */
char fscanc(tFD fd);

/**
 * Puts the given character back to the buffer for STDIN. If the buffer is full, the character
 * will be ignored.
 *
 * @param c the character
 */
void scanback(char c);

/**
 * Puts the given character back to the buffer for <fd>. If the buffer is full, the character
 * will be ignored.
 *
 * @param fd the file-descriptor
 * @param c the character
 */
void fscanback(tFD fd,char c);

/**
 * Reads max. <max> from STDIN (or till EOF or newline) into the given line-buffer
 *
 * @param line the buffer
 * @param max the maximum number of chars to read
 * @return the number of read chars
 */
u32 scans(char *line,u32 max);

/**
 * Reads max. <max> from <fd> (or till EOF or newline) into the given line-buffer
 *
 * @param fd the file-descriptor
 * @param line the buffer
 * @param max the maximum number of chars to read
 * @return the number of read chars
 */
u32 fscans(tFD fd,char *line,u32 max);

/**
 * Reads data in the specified format from STDIN. Supports:
 * 	%d: signed integer
 * 	%u: unsigned integer, base 10
 * 	%o: unsigned integer, base 8
 * 	%x: unsigned integer, base 16
 * 	%b: unsigned integer, base 2
 * 	%s: string
 * 	%c: character
 *
 * Additionally you can specify the max. length:
 * 	%2d
 * 	%10s
 *  ...
 *
 * @param fmt the format
 * @return the number of matched variables
 */
u32 scanf(const char *fmt,...);

/**
 * Reads data in the specified format from <fd>. See scanf().
 *
 * @param fd the file-descriptor
 * @param fmt the format
 * @return the number of matched variables
 */
u32 fscanf(tFD fd,const char *fmt,...);

/**
 * Reads data in the specified format from STDIN with the given argument-list
 *
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of matched variables
 */
u32 vscanf(const char *fmt,va_list ap);

/**
 * Reads data in the specified format from <fd> with the given argument-list
 *
 * @param fd the file-descriptor
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of matched variables
 */
u32 vfscanf(tFD fd,const char *fmt,va_list ap);

#endif /* BUFIO_H_ */
