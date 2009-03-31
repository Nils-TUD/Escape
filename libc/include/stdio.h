/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef STDIO_H_
#define STDIO_H_

#include <types.h>

typedef tFD FILE;

/**
 * Opens the given file and returns the stream or NULL on error. Mode may be:
 * "r"	- create textfile for reading
 * "w"	- create textfile for writing; delete old content
 * "a"	- open textfile to append text; create if not exists
 * "r+"	- create textfile for change (read and write)
 * "w+"	- create textfile for change; delete old content
 * "a+" - open textfile for change; create if not exists
 *
 * "change" means that it is allowed to read and write the file at the same time.
 *
 * @param filename the file to open
 * @param mode the mode
 * @return the stream or NULL
 */
FILE *fopen(const char *filename,const char *mode);

/**
 * Closes the given stream
 *
 * @param stream the stream
 * @return 0 on success or EOF
 */
int fclose(FILE *stream);

/**
 * The  function  fflush  forces a write of all buffered data for the given output
 *
 * @param stream the stream
 * @return 0 on success or EOF
 */
int fflush(FILE *stream);

/*
fprintf	Write formatted output to stream (function)
fscanf	Read formatted data from stream (function)
printf	Print formatted data to stdout (function)
scanf	Read formatted data from stdin (function)
sprintf	Write formatted data to string (function)
sscanf	Read formatted data from string (function)
vfprintf	Write formatted variable argument list to stream (function)
vprintf	Print formatted variable argument list to stdout (function)
vsprintf	Print formatted variable argument list to string (function)

Character input/output:
fgetc	Get character from stream (function)
fgets	Get string from stream (function)
fputc	Write character to stream (function)
fputs	Write string to stream (function)
getc	Get character from stream (function)
getchar	Get character from stdin (function)
gets	Get string from stdin (function)
putc	Write character to stream (function)
putchar	Write character to stdout (function)
puts	Write string to stdout (function)
ungetc	Unget character from stream (function)*/

/**
 * Prints "<msg>: <lastError>"
 *
 * @param msg the prefix of the message
 */
void perror(const char *msg);

#endif /* STDIO_H_ */
