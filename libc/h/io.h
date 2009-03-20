/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef IO_H_
#define IO_H_

#include "common.h"
#include <stdarg.h>

/* IO flags */
#define IO_READ			1
#define IO_WRITE		2

/* file descriptors for stdin, stdout and stderr */
#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

/**
 * Inits IO. That means stdin, stdout and stderr will be created and some other stuff
 */
bool initIO(void);

/**
 * Reads one character from the vterm
 *
 * @return the character
 */
s8 readChar(void);

/**
 * Reads a line from the vterm
 *
 * @param buffer the buffer to use
 * @param max the maximum number of chars to read
 * @return the number of read chars
 */
u16 readLine(s8 *buffer,u16 max);

/**
 * Handles the default escape-codes for readLine. This is just intended for the programs
 * that want to support additional escape-codes, such as the shell.
 * It handles \b, VK_LEFT, VK_RIGHT, VK_HOME and VK_END.
 *
 * @param buffer the buffer with read characters
 * @param cursorPos the current cursor-position in the buffer (may be changed)
 * @param charcount the number of read characters so far (may be changed)
 * @param c the character
 * @param keycode if c was an escape-code keycode will be set
 * @param modifier if c was an escape-code modifier will be set
 * @return true if the escape-code was handled
 */
bool handleDefaultEscapeCodes(s8 *buffer,u16 *cursorPos,u16 *charcount,s8 c,u8 *keycode,u8 *modifier);

/**
 * Prints the given character to the current position on the screen
 *
 * @param c the character
 */
void putchar(s8 c);

/**
 * Prints the given string on the screen
 *
 * @param str the string
 */
void puts(cstring str);

/**
 * Prints the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 */
void printu(u32 n,u8 base);

/**
 * Prints the given signed 32-bit integer in base 10
 *
 * @param n the integer
 */
void printn(s32 n);

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
void printf(cstring fmt,...);

/**
 * Same as printf, but with the va_list as argument
 *
 * @param fmt the format
 * @param ap the argument-list
 */
void vprintf(cstring fmt,va_list ap);

/**
 * Flushes the buffer
 */
void flush(void);

/**
 * Request the given IO-port
 *
 * @param port the port
 * @return a negative error-code or 0 if successfull
 */
s32 requestIOPort(u16 port);

/**
 * Request the given IO-ports
 *
 * @param start the start-port
 * @param count the number of ports to reserve
 * @return a negative error-code or 0 if successfull
 */
s32 requestIOPorts(u16 start,u16 count);

/**
 * Releases the given IO-port
 *
 * @param port the port
 * @return a negative error-code or 0 if successfull
 */
s32 releaseIOPort(u16 port);

/**
 * Releases the given IO-ports
 *
 * @param start the start-port
 * @param count the number of ports to reserve
 * @return a negative error-code or 0 if successfull
 */
s32 releaseIOPorts(u16 start,u16 count);

/**
 * Outputs the byte <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void outByte(u16 port,u8 val);

/**
 * Outputs the word <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void outWord(u16 port,u16 val);

/**
 * Reads a byte from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern u8 inByte(u16 port);

/**
 * Reads a word from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern u16 inWord(u16 port);

/**
 * Opens the given path with given mode and returns the associated file-descriptor
 *
 * @param path the path to open
 * @param mode the mode
 * @return the file-descriptor; negative if error
 */
s32 open(cstring path,u8 mode);

/**
 * Reads count bytes from the given file-descriptor into the given buffer and returns the
 * actual read number of bytes.
 *
 * @param fd the file-descriptor
 * @param buffer the buffer to fill
 * @param count the number of bytes
 * @return the actual read number of bytes; negative if an error occurred
 */
s32 read(tFD fd,void *buffer,u32 count);

/**
 * Writes count bytes from the given buffer into the given fd and returns the number of written
 * bytes.
 *
 * @param fd the file-descriptor
 * @param buffer the buffer to read from
 * @param count the number of bytes to write
 * @return the number of bytes written; negative if an error occurred
 */
s32 write(tFD fd,void *buffer,u32 count);

/**
 * Duplicates the given file-descriptor
 *
 * @param fd the file-descriptor
 * @return the error-code or the new file-descriptor
 */
s32 dupFd(tFD fd);

/**
 * Redirects <src> to <dst>. <src> will be closed. Note that both fds have to exist!
 *
 * @param src the source-file-descriptor
 * @param dst the destination-file-descriptor
 * @return the error-code or 0 if successfull
 */
s32 redirFd(tFD src,tFD dst);

/**
 * Closes the given file-descriptor
 *
 * @param fd the file-descriptor
 */
void close(tFD fd);

#endif /* IO_H_ */
