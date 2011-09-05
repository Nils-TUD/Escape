/**
 * $Id: io.h 239 2011-08-30 18:28:06Z nasmussen $
 */

#ifndef FORMAT_H_
#define FORMAT_H_

/**
 * This module builds a layer on all needed printf/scanf and conversion-functions to hide
 * the differences of 32- and 64-bit architectures. That means you can simply print or read
 * an octa, for example, without thinking about its representation on the used platform.
 *
 * To achieve this it adds the length-options O,T,W and B to the existing h,l and L.
 * So for example:
 * printf("%016Ox",1234);
 * This would print 1234 as an octa with 16 digits padded with zeros in hexadecimal.
 */

#include <stdarg.h>
#include <stdio.h>
#include "common.h"

/**
 * Uses strtoul or strtoull, depending on the platform, to parse an octa in <base> from <str>.
 */
octa mstrtoo(const char *str,char **endptr,int base);

/**
 * Uses scanf,sscanf or fscanf and converts the format-string correspondingly.
 */
int mscanf(const char *fmt,...);
int msscanf(const char *str,const char *fmt,...);
int mfscanf(FILE *f,const char *fmt,...);

/**
 * Uses printf,vprintf,snprintf,vsnprintf,fprintf or vfprintf and converts the format-string
 * correspondingly.
 */
int mprintf(const char *fmt,...);
int mvprintf(const char *fmt,va_list ap);
int msnprintf(char *str,size_t size,const char *fmt,...);
int mvsnprintf(char *str,size_t size,const char *fmt,va_list ap);
int mfprintf(FILE *f,const char *fmt,...);
int mvfprintf(FILE *f,const char *fmt,va_list ap);

#endif /* FORMAT_H_ */
