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

#ifndef STDIO_H_
#define STDIO_H_

#include <types.h>
#include <esc/fileio.h>

#define EOF IO_EOF
typedef tFile FILE;

/* std-streams */
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/**
 * Opens the file whose name is specified in the parameter filename and associates it with a
 * stream that can be identified in future operations by the FILE object whose pointer is returned.
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
 * @return a FILE object if successfull or NULL if failed
 */
extern FILE *fopen(const char *filename,const char *mode);

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
extern size_t fread(void *ptr,size_t size,size_t count,FILE *file);

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
extern size_t fwrite(const void *ptr,size_t size,size_t count,FILE *file);

/**
 * The  function  fflush  forces a write of all buffered data for the given output
 *
 * @param stream the stream
 * @return 0 on success or EOF
 */
extern int fflush(FILE *stream);

/**
 * Closes the given stream
 *
 * @param stream the stream
 * @return 0 on success or EOF
 */
extern int fclose(FILE *stream);

/**
 * Prints the given character to STDOUT
 *
 * @param c the character
 * @return the character or IO_EOF if failed
 */
int putchar(int c);

/**
 * Prints the given character to <file>
 *
 * @param file the file
 * @param c the character
 * @return the character or IO_EOF if failed
 */
int putc(FILE *file,int c);

/**
 * Prints the given string to STDOUT
 *
 * @param str the string
 * @return the number of written chars
 */
int puts(const char *str);

/**
 * Prints the given string to <file>
 *
 * @param file the file
 * @param str the string
 * @return the number of written chars
 */
int fputs(FILE *file,const char *str);

/**
 * Formated output to STDOUT. Supports:
 * TODO refresh comment here
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
extern int printf(const char *fmt,...);

/**
 * Like printf(), but prints to <file>
 *
 * @param file the file
 * @param fmt the format
 * @return the number of written chars
 */
extern int fprintf(FILE *file,const char *fmt,...);

/**
 * Like printf(), but prints to the given string
 *
 * @param str the string to print to
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of written chars
 */
extern int sprintf(char *str,const char *fmt,...);

/**
 * Like printf(), but lets you specify the argument-list
 *
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of written chars
 */
extern int vprintf(const char *fmt,va_list ap);

/**
 * Like vprintf(), but prints to <file>
 *
 * @param file the file
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of written chars
 */
extern int vfprintf(FILE *file,const char *fmt,va_list ap);

/**
 * Like vprintf(), but prints to <str>
 *
 * @param str the string to print to
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of written chars
 */
extern int vsprintf(char *str,const char *fmt,va_list ap);

/**
 * Reads one char from STDIN
 *
 * @return the character or IO_EOF
 */
int getchar(void);

/**
 * Reads one char from <file>
 *
 * @param file the file
 * @return the character or IO_EOF
 */
int getc(FILE *file);

/**
 * Puts the given character back to the buffer for <file>. If the buffer is full, the character
 * will be ignored.
 *
 * @param c the character
 * @param file the file
 * @return 0 on success or IO_EOF
 */
int ungetc(int c,FILE *file);

/**
 * Reads from STDIN (or till EOF) into the given buffer
 *
 * @param buffer the buffer
 * @return the number of read chars
 */
int gets(char *buffer);

/**
 * Reads max. <max> from <file> (or till EOF) into the given buffer
 *
 * @param str the buffer
 * @param max the maximum number of chars to read
 * @param file the file
 * @return the number of read chars
 */
char *fgets(char *str,int max,FILE *file);

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
extern int scanf(const char *fmt,...);

/**
 * Reads data in the specified format from <file>. See scanf().
 *
 * @param file the file
 * @param fmt the format
 * @return the number of matched variables
 */
extern int fscanf(FILE *file,const char *fmt,...);

/**
 * Reads data in the specified format from <str>. See scanf().
 *
 * @param str the string
 * @param fmt the format
 * @return the number of matched variables
 */
extern int sscanf(const char *str,const char *fmt,...);

/**
 * Prints "<msg>: <lastError>"
 *
 * @param msg the prefix of the message
 */
void perror(const char *msg);

#endif /* STDIO_H_ */
