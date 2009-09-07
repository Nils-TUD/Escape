/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef FILEIO_H_
#define FILEIO_H_

#include <esc/common.h>
#include <stdarg.h>

#define IO_EOF	-1

/* the user doesn't need to not know the real type of tFile... */
typedef void tFile;

#ifdef __cplusplus
extern "C" {
#endif

/* std-streams */
extern tFile *stdin;
extern tFile *stdout;
extern tFile *stderr;

/**
 * The same as escc_get() but reads the escape-code from <f>. Assumes that the last read char
 * was '\033'.
 *
 * @param f the file
 * @param n1 will be set to the first argument (ESCC_ARG_UNUSED if unused)
 * @param n2 will be set to the second argument (ESCC_ARG_UNUSED if unused)
 * @return the scanned escape-code (ESCC_*)
 */
s32 freadesc(tFile *f,s32 *n1,s32 *n2);

/**
 * Opens the file whose name is specified in the parameter filename and associates it with a
 * stream that can be identified in future operations by the tFile object whose pointer is returned.
 * The operations that are allowed on the stream and how these are performed are defined by the
 * mode parameter.
 *
 * @param filename the file to open
 * @param mode string containing a file access modes. It can be:
 * 	"r"		Open a file for reading. The file must exist.
 * 	"w"		Create an empty file for writing. If a file with the same name already exists its content
 * 			is erased and the file is treated as a new empty file.
 * 	"a"		Append to a file. Writing operations append data at the end of the file. The file is
 * 			created if it does not exist.
 * 	"r+"	Open a file for update both reading and writing. The file must exist.
 * 	"w+"	Create an empty file for both reading and writing. If a file with the same name
 * 			already exists its content is erased and the file is treated as a new empty file.
 * 	"a+"	Open a file for reading and appending. All writing operations are performed at the
 * 			end of the file, protecting the previous content to be overwritten. You can
 * 			reposition (fseek, rewind) the internal pointer to anywhere in the file for
 * 			reading, but writing operations will move it back to the end of file.
 * 			The file is created if it does not exist.
 * @return a tFile object if successfull or NULL if failed
 */
tFile *fopen(const char *filename,const char *mode);

/**
 * Reads an array of count elements, each one with a size of size bytes, from the stream and
 * stores them in the block of memory specified by ptr.
 * The postion indicator of the stream is advanced by the total amount of bytes read.
 * The total amount of bytes read if successful is (size * count).
 *
 * @param ptr Pointer to a block of memory with a minimum size of (size*count) bytes.
 * @param size Size in bytes of each element to be read.
 * @param count Number of elements, each one with a size of size bytes.
 * @param file Pointer to a tFile object that specifies an input stream.
 * @return total number of read elements (maybe less than <count>, if EOF is reached or an error
 * 	occurred)
 */
u32 fread(void *ptr,u32 size,u32 count,tFile *file);

/**
 * Writes an array of count elements, each one with a size of size bytes, from the block of memory
 * pointed by ptr to the current position in the stream.
 * The postion indicator of the stream is advanced by the total number of bytes written.
 *
 * @param ptr Pointer to the array of elements to be written.
 * @param size Size in bytes of each element to be written.
 * @param count Number of elements, each one with a size of size bytes.
 * @param file Pointer to a tFile object that specifies an output stream.
 * @return total number of written elements (if it differs from <count> an error is occurred)
 */
u32 fwrite(const void *ptr,u32 size,u32 count,tFile *file);

/**
 * If the given stream was open for writing and the last i/o operation was an output operation,
 * any unwritten data in the output buffer is written to the file.
 * If the argument is a null pointer, all open files are flushed.
 *
 * @param file Pointer to a tFile object that specifies a buffered stream.
 * @return 0 on success or EOF
 */
s32 fflush(tFile *file);

/**
 * Closes the file associated with the stream and disassociates it.
 * All internal buffers associated with the stream are flushed: the content of any unwritten
 * buffer is written and the content of any unread buffer is discarded.
 * Even if the call fails, the stream passed as parameter will no longer be associated with the file.
 *
 * @param file Pointer to a tFile object that specifies the stream to be closed.
 * @return 0 on success or EOF
 */
s32 fclose(tFile *file);

/**
 * Prints the given character to STDOUT
 *
 * @param c the character
 * @return the character or IO_EOF if failed
 */
char printc(char c);

/**
 * Prints the given character to <file>
 *
 * @param file the file
 * @param c the character
 * @return the character or IO_EOF if failed
 */
char fprintc(tFile *file,char c);

/**
 * Prints the given string to STDOUT
 *
 * @param str the string
 * @return the number of written chars
 */
s32 prints(const char *str);

/**
 * Prints the given string to <file>
 *
 * @param file the file
 * @param str the string
 * @return the number of written chars
 */
s32 fprints(tFile *file,const char *str);

/**
 * Prints the given signed integer to STDOUT
 *
 * @param n the number
 * @return the number of written chars
 */
s32 printn(s32 n);

/**
 * Prints the given signed integer to <file>
 *
 * @param n the number
 * @return the number of written chars
 */
s32 fprintn(tFile *file,s32 n);

/**
 * Prints the given unsigned integer to the given base to STDOUT
 *
 * @param u the number
 * @param base the base (2 .. 16)
 * @return the number of written chars
 */
s32 printu(s32 n,u8 base);

/**
 * Prints the given unsigned integer to the given base to <file>
 *
 * @param u the number
 * @param base the base (2 .. 16)
 * @return the number of written chars
 */
s32 fprintu(tFile *file,s32 u,u8 base);

/**
 * Prints "<prefix>: <lastError>" to stderr
 *
 * @param prefix the prefix of the message
 * @return the number of written chars
 */
s32 printe(const char *prefix,...);

/**
 * The same as printe(), but with argument-pointer specified
 *
 * @param prefix the prefix of the message
 * @param ap the argument-pointer
 * @return the number of written chars
 */
s32 vprinte(const char *prefix,va_list ap);

/**
 * Formated output to STDOUT.
 * You can format values with: %[flags][width][.precision][length]specifier
 *
 * Flags:
 *  "-"		: Left-justify within the given field width; Right justification is the default
 *  "+"		: Forces to precede the result with a plus or minus sign (+ or -) even for
 *  		  positive numbers
 *  " "		: If no sign is going to be written, a blank space is inserted before the value.
 *  "#"		: Used with o, x or X specifiers the value is preceeded with 0, 0x or 0X respectively
 *  "0"		: Left-pads the number with zeroes (0) instead of spaces, where padding is specified
 *
 * Width:
 *  [0-9]+	: Minimum number of characters to be printed. If the value to be printed is shorter
 *  		  than this number, the result is padded with blank spaces.
 *  "*"		: The width is not specified in the format string, but as an additional integer value
 *  		  argument preceding the argument that has to be formatted.
 *
 * Precision:
 * 	[0-9]+	: For integer specifiers (d, i, o, u, x, X): precision specifies the minimum number of
 * 			  digits to be written. If the value to be written is shorter than this number, the
 * 			  result is padded with leading zeros.
 * 	"*"		: The precision is not specified in the format string, but as an additional integer
 * 			  value argument preceding the argument that has to be formatted.
 *
 * Length:
 * 	"h"		: The argument is interpreted as a short int or unsigned short int (only applies to
 * 			  integer specifiers: i, d, o, u, x and X).
 * 	"l"		: The argument is interpreted as a long int, unsigned long int or double
 * 	"L"		: The argument is interpreted as a long long int or unsigned long long int
 *
 * Specifier:
 * 	"c"		: a character
 * 	"d"		: signed decimal integer
 * 	"i"		: alias of "d"
 * 	"f"		: decimal floating point
 * 	"o"		: unsigned octal integer
 * 	"u"		: unsigned decimal integer
 * 	"x"		: unsigned hexadecimal integer
 * 	"X"		: unsigned hexadecimal integer (capical letters)
 * 	"b"		: unsigned binary integer
 * 	"p"		: pointer address
 * 	"s"		: string of characters
 * 	"n"		: prints nothing. The argument must be a pointer to a signed int, where the number of
 * 			  characters written so far is stored.
 *
 * @param fmt the format
 * @return the number of written chars
 */
s32 printf(const char *fmt,...);

/**
 * Like printf(), but prints to <file>
 *
 * @param file the file
 * @param fmt the format
 * @return the number of written chars
 */
s32 fprintf(tFile *file,const char *fmt,...);

/**
 * Like printf(), but prints to the given string
 *
 * @param str the string to print to
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of written chars
 */
s32 sprintf(char *str,const char *fmt,...);

/**
 * Like sprintf(), but prints max <max> chars into the buffer
 *
 * @param str the string to print to
 * @param max the maximum number of chars to print into the buffer (-1 = unlimited)
 * @param fmt the format
 * @return the number of written chars
 */
s32 snprintf(char *str,u32 max,const char *fmt,...);

/**
 * Like printf(), but lets you specify the argument-list
 *
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of written chars
 */
s32 vprintf(const char *fmt,va_list ap);

/**
 * Like vprintf(), but prints to <file>
 *
 * @param file the file
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of written chars
 */
s32 vfprintf(tFile *file,const char *fmt,va_list ap);

/**
 * Like vprintf(), but prints to <str>
 *
 * @param str the string to print to
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of written chars
 */
s32 vsprintf(char *str,const char *fmt,va_list ap);

/**
 * Like vsprintf(), but prints max <max> chars into the buffer
 *
 * @param str the string to print to
 * @param max the maximum number of chars to print into the buffer (-1 = unlimited)
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of written chars
 */
s32 vsnprintf(char *str,u32 max,const char *fmt,va_list ap);

/**
 * Flushes the output-channel of STDOUT
 */
void flush(void);

/**
 * Reads one char from STDIN
 *
 * @return the character or IO_EOF
 */
char scanc(void);

/**
 * Reads one char from <file>
 *
 * @param file the file
 * @return the character or IO_EOF
 */
char fscanc(tFile *file);

/**
 * Puts the given character back to the buffer for STDIN. If the buffer is full, the character
 * will be ignored.
 *
 * @param c the character
 * @return 0 on success or IO_EOF
 */
s32 scanback(char c);

/**
 * Puts the given character back to the buffer for <file>. If the buffer is full, the character
 * will be ignored.
 *
 * @param file the file
 * @param c the character
 * @return 0 on success or IO_EOF
 */
s32 fscanback(tFile *file,char c);

/**
 * Reads max. <max> from STDIN (or till EOF) into the given buffer
 *
 * @param buffer the buffer
 * @param max the maximum number of chars to read
 * @return the number of read chars
 */
s32 scans(char *buffer,u32 max);

/**
 * Reads max. <max> from <file> (or till EOF) into the given buffer
 *
 * @param file the file
 * @param buffer the buffer
 * @param max the maximum number of chars to read
 * @return the number of read chars
 */
s32 fscans(tFile *file,char *buffer,u32 max);

/**
 * Reads max. <max> from STDIN (or till EOF or newline) into the given line-buffer
 *
 * @param line the buffer
 * @param max the maximum number of chars to read
 * @return the number of read chars
 */
s32 scanl(char *line,u32 max);

/**
 * Reads max. <max> from <file> (or till EOF or newline) into the given line-buffer
 *
 * @param file the file
 * @param line the buffer
 * @param max the maximum number of chars to read
 * @return the number of read chars
 */
s32 fscanl(tFile *file,char *line,u32 max);

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
s32 scanf(const char *fmt,...);

/**
 * Reads data in the specified format from <file>. See scanf().
 *
 * @param file the file
 * @param fmt the format
 * @return the number of matched variables
 */
s32 fscanf(tFile *file,const char *fmt,...);

/**
 * Reads data in the specified format from <str>. See scanf().
 *
 * @param str the string
 * @param fmt the format
 * @return the number of matched variables
 */
s32 sscanf(const char *str,const char *fmt,...);

/**
 * Reads data in the specified format from STDIN with the given argument-list
 *
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of matched variables
 */
s32 vscanf(const char *fmt,va_list ap);

/**
 * Reads data in the specified format from <file> with the given argument-list
 *
 * @param file the file
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of matched variables
 */
s32 vfscanf(tFile *file,const char *fmt,va_list ap);

/**
 * Reads data in the specified format from <str> with the given argument-list.
 *
 * @param str the string
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of matched variables
 */
s32 vsscanf(const char *str,const char *fmt,va_list ap);

/**
 * Determines the width of the given signed 32-bit integer in base 10
 *
 * @param n the integer
 * @return the width
 */
u8 getnwidth(s32 n);

/**
 * Determines the width of the given signed 64-bit integer in base 10
 *
 * @param n the integer
 * @return the width
 */
u8 getlwidth(s64 n);

/**
 * Determines the width of the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 * @return the width
 */
u8 getuwidth(u32 n,u8 base);

/**
 * Determines the width of the given unsigned 64-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 * @return the width
 */
u8 getulwidth(u64 n,u8 base);

/**
 * The internal printf-routine. Should NOT BE USED by user-programs. This is just intended
 * for libcpp to prevent a duplicate implementation there.
 */
s32 doVfprintf(void *buf,const char *fmt,va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* FILEIO_H_ */
