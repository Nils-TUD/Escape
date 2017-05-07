/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#pragma once

#include <sys/common.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The atof function converts the initial portion of the string pointed to by nptr to
 * double representation. Except for the behavior on error, it is equivalent to
 * 	strtod(nptr, (char **)NULL)
 *
 * @param nptr the string
 * @return the double
 */
double atof(const char *nptr);

/**
 * The atoi() function converts str into an integer, and returns that integer. str should start
 * with whitespace or some sort of number, and atoi() will stop reading from str as soon as
 * a non-numerical character has been read.
 *
 * @param str the string
 * @return the integer
 */
int atoi(const char *nptr);

/**
 * An alias of atoi()
 *
 * @param nptr the string
 * @return the integer
 */
long atol(const char *nptr);

/**
 * The same as atoi(), but for a long long int
 *
 * @param nptr the string
 * @return the integer
 */
llong atoll(const char *nptr);

/**
 * Converts the given signed integer to a string
 *
 * @param target the target-string (needs max. 12 elements)
 * @param targetSize the size of <target>
 * @param n the integer
 * @return the number of chars written
 */
size_t itoa(char *target,size_t targetSize,int n);

/**
 * The memchr() function looks for the first occurrence of c within count characters in
 * the array pointed to by buffer.
 *
 * @param buffer the buffer to search in
 * @param c the element to search for
 * @param count the number of characters to loop through
 * @return NULL if not found or the pointer to the found element
 */
void *memchr(const void *buffer,int c,size_t count);

/**
 * Copies <len> words from <src> to <dest>
 *
 * @param dest the destination-address
 * @param src the source-address
 * @param len the number of words to copy
 */
void *memcpy(void *dest,const void *src,size_t len);

/**
 * Compares <count> words of <str1> and <str2> and returns whether they are equal
 *
 * @param str1 the first data
 * @param str2 the second data
 * @param count the number of words
 * @return -1 if <str1> lt <str2>, 0 if equal and 1 if <str1> gt <str2>
 */
int memcmp(const void *str1,const void *str2,size_t count);

/**
 * Searches for <needle> in <haystack> with given lengths.
 *
 * @param haystack the memory to search in
 * @param hslen the length of the haystack
 * @param needle the memory to search for
 * @param nlen the length of the needle
 * @return the pointer to the found needle or NULL if not found
 */
void *memmem(const void *haystack,size_t hslen,const void *needle,size_t nlen);

/**
 * Swaps the <n>-byte big values <a> and <b>
 *
 * @param a pointer to the first value
 * @param b pointer to the second value
 * @param n the number of bytes
 */
void memswp(void *a,void *b,size_t n);

/**
 * Sets all bytes in memory beginning at <addr> and ending at <addr> + <count>
 * to <value>.
 *
 * @param addr the starting address
 * @param value the value to set
 * @param count the number of bytes
 * @return <addr>
 */
void *memset(void *addr,int value,size_t count);

/**
 * Sets <count> bytes starting at <addr> to 0.
 *
 * @param addr the starting address
 * @param count the number of bytes
 */
static inline void memclear(void *addr,size_t count) {
	memset(addr,0,count);
}

/**
 * Copies the values of <count> bytes from the location pointed by <src> to the memory block pointed
 * by <dest>. Copying takes place as if an intermediate buffer was used, allowing the destination
 * and source to overlap.
 *
 * @param dest the destination
 * @param src the source
 * @param count the number of bytes to move
 * @return the destination
 */
void *memmove(void *dest,const void *src,size_t count);

/**
 * The strcpy() function copies characters in the string <from> to the string <to>, including the
 * null termination.
 * Note that strcpy() does not perform bounds checking, and thus risks overrunning from or to.
 * For a similar (and safer) function that includes bounds checking, see strncpy().
 *
 * @param to the target
 * @param from the source
 * @return to
 */
char *strcpy(char *to,const char *from);

/**
 * The strncpy() function copies at most <count> characters of <from> to the string <to>.
 * If <from> has less than <count> characters, the remainder is padded with '\0' characters.
 * Note that it will NOT null-terminate the string if <from> has >= <count> chars!
 *
 * @param to the target
 * @param from the source
 * @param count the number of chars to copy
 * @return the target string
 */
char *strncpy(char *to,const char *from,size_t count);

/**
 * Copies at most <count> characters of <from> to the string <to>. It stops when either <from> is
 * over or count has reached 1. In every case, the string in <to> is null-terminated.
 *
 * @param to the target
 * @param from the source
 * @param size the size of the target
 * @return the number of written characters (without NULL-termination)
 */
size_t strnzcpy(char *to,const char *from,size_t size);

/**
 * The strcat() function concatenates str2 onto the end of str1, and returns str1.
 *
 * @param str1 the string to append to
 * @param str2 the string to append
 * @return pointer to str1
 */
char *strcat(char *str1,const char *str2);

/**
 * The function strncat() concatenates at most count characters of str2 onto str1,
 * adding a null termination
 *
 * @param str1 the string to append to
 * @param str2 the string to append
 * @param count the number of elements to concatenate to str1
 * @return resulting string
 */
char *strncat(char *str1,const char *str2,size_t count);

/**
 * The function strcmp() compares str1 and str2, then returns:
 * 	- less than 0		: str1 is less than str2
 *  - equal to 0		: str1 is equal to str2
 *  - greater than 0	: str1 is greater than str2
 *
 * @param str1 the first string
 * @param str2 the second string
 * @return the result
 */
int strcmp(const char *str1,const char *str2);

/**
 * Compares at most count characters of str1 and str2. The return value is as follows:
 * 	- less than 0		: str1 is less than str2
 *  - equal to 0		: str1 is equal to str2
 *  - greater than 0	: str1 is greater than str2
 *
 * @param str1 the first string
 * @param str2 the second string
 * @param count the number of chars to compare
 * @return the result
 */
int strncmp(const char *str1,const char *str2,size_t count);

/**
 * The function strcasecmp() compares str1 and str2 ignoring case, then returns:
 * 	- less than 0		: str1 is less than str2
 *  - equal to 0		: str1 is equal to str2
 *  - greater than 0	: str1 is greater than str2
 *
 * @param str1 the first string
 * @param str2 the second string
 * @return the result
 */
int strcasecmp(const char *str1,const char *str2);

/**
 * Compares at most count characters of str1 and str2 ignoring case. The return value is as follows:
 * 	- less than 0		: str1 is less than str2
 *  - equal to 0		: str1 is equal to str2
 *  - greater than 0	: str1 is greater than str2
 *
 * @param str1 the first string
 * @param str2 the second string
 * @param count the number of chars to compare
 * @return the result
 */
int strncasecmp(const char *str1,const char *str2,size_t count);

/**
 * The function strchr() returns a pointer to the first occurence of ch in str, or NULL
 * if ch is not found.
 *
 * @param str the string
 * @param ch the character
 * @return NULL if not found or the pointer to the found character
 */
char *strchr(const char *str,int ch);

/**
 * Returns the index of the first occurrences of ch in str or strlen(str) if ch is not found.
 *
 * @param str the string
 * @param ch the character
 * @return the index of the character or strlen(str) if not found
 */
int strchri(const char *str,int ch);

/**
 * @param str the string to search
 * @param ch the character to search for
 * @return a pointer to the last occurrence of ch in str, or NULL if no match is found
 */
char *strrchr(const char *str,int ch);

/**
 * The function strstr() returns a pointer to the first occurrence of str2 in str1, or NULL
 * if no match is found. If the length of str2 is zero, then strstr () will simply return str1.
 *
 * @param str1 the string to search
 * @param str2 the substring to search for
 * @return the pointer to the match or NULL
 */
char *strstr(const char *str1,const char *str2);

/**
 * The function strcasestr() returns a pointer to the first occurrence of str2 in str1, or NULL
 * if no match is found. If the length of str2 is zero, then strstr () will simply return str1.
 * In contrast to strstr(), it uses case-insensitive comparison.
 *
 * @param str1 the string to search
 * @param str2 the substring to search for
 * @return the pointer to the match or NULL
 */
char *strcasestr(const char *str1,const char *str2);

/**
 * Returns the length of the initial portion of str1 which consists only of characters that are
 * part of str2.
 *
 * @param str1 the string to search in
 * @param str2 the character list
 * @return the length of the initial portion of str1 containing only characters that appear in str2.
 */
size_t strspn(const char *str1,const char *str2);

/**
 * Scans str1 for the first occurrence of any of the characters that are part of str2, returning
 * the number of characters of str1 read before this first occurrence.
 * The search includes the terminating null-characters, so the function will return the length
 * of str1 if none of the characters of str2 are found in str1.
 *
 * @param str1 the string to search in
 * @param str2 the character list
 * @return the length of the initial part of str1 not containing any of the characters that are
 * 	part of str2.
 */
size_t strcspn(const char *str1,const char *str2);

/**
 * The function returns a pointer to the first occurrence in str1 of any character in
 * str2, or NULL if no such characters are present.
 *
 * @param str1 the string to search
 * @param str2 the characters to find
 * @return the first occurrence or NULL
 */
char *strpbrk(const char *str1,const char *str2);

/**
 * A sequence of calls to this function split str into tokens, which are sequences of contiguous
 * characters separated by any of the characters that are part of delimiters.
 * On a first call, the function expects a C string as argument for str, whose first character
 * is used as the starting location to scan for tokens. In subsequent calls, the function expects
 * a null pointer and uses the position right after the end of last token as the new starting
 * location for scanning.
 * To determine the beginning and the end of a token, the function first scans from the starting
 * location for the first character not contained in delimiters (which becomes the beginning of
 * the token). And then scans starting from this beginning of the token for the first character
 * contained in delimiters, which becomes the end of the token.
 * This end of the token is automatically replaced by a null-character by the function, and the
 * beginning of the token is returned by the function.
 * Once the terminating null character of str has been found in a call to strtok, all subsequent
 * calls to this function with a null pointer as the first argument return a null pointer.
 *
 * @param str the string to tokenize (not NULL for first call; NULL for all other)
 * @param delimiters a list of delimiter-chars
 * @return the beginning of the token or NULL if the end is reached
 */
char *strtok(char *str,const char *delimiters);

/**
 * Cuts out the first count characters in the given string. That means all characters behind
 * will be moved count chars back.
 *
 * @param str the string
 * @param count the number of chars to remove
 * @return the string
 */
char *strcut(char *str,size_t count);

/**
 * @param str the string
 * @return the length of str (determined by the number of characters before null termination).
 */
size_t strlen(const char *str);

/**
 * Determines the length of the string to the given maximum length. That means the function
 * does not walk further than (str + max).
 *
 * @param str the string
 * @param max the maximum number of characters
 * @return the length of the string or -1 if the string is too long
 */
ssize_t strnlen(const char *str,ssize_t max);

/**
 * Checks whether the given string contains just alphanumeric characters
 *
 * @param str the string
 * @return true if so
 */
bool isalnumstr(const char *str);

/**
 * Interprets the value of errnum generating a string describing the error that usually
 * generates that error number value in calls to functions of the C library.
 * The returned pointer points to a statically allocated string, which shall not be modified by
 * the program. Further calls to this function will overwrite its content.
 *
 * @param errnum the error-code
 * @return the error-message
 */
char *strerror(int errnum);

/**
 * The strdup() function returns a pointer to a new string which is a duplicate of the string s.
 * Memory for the new string is obtained with malloc(3), and can be freed with free(3).
 *
 * @param s the string
 * @return the duplicated string or NULL
 */
char *strdup(const char *s);

/**
 * Checks whether <str> matches <pattern>. Pattern may contain '*' which acts as a wildcard.
 *
 * @param pattern the pattern
 * @param str the string to match
 * @return true if it matches
 */
bool strmatch(const char *pattern,const char *str);

/**
 * The strndup() function is similar, but only copies at most n characters.  If s is longer than
 * n, only n characters are copied, and a terminating null byte ('\0') is added.
 *
 * @param s the string
 * @param n the number of chars
 * @return the duplicated string or NULL
 */
char *strndup(const char *s,size_t n);

/**
 * The  strtol()  function  converts  the  initial part of the string in nptr to a long integer
 * value according to the given base, which must be between 2 and 36 inclusive, or be the special
 * value 0.
 *
 * The string may begin with an arbitrary amount of white space (as determined by isspace(3))
 * followed by a single  optional '+'  or  '-' sign.  If base is zero or 16, the string may then
 * include a "0x" prefix, and the number will be read in base 16; otherwise, a zero base is taken
 * as 10 (decimal) unless the next character is '0', in which case  it  is  taken  as  8 (octal).
 *
 * The remainder of the string is converted to a long int value in the obvious manner, stopping at
 * the first character which is not a valid digit in the given base.  (In bases above 10, the
 * letter 'A' in either upper or lower case represents  10, 'B' represents 11, and so forth,
 * with 'Z' representing 35.)
 *
 * If endptr is not NULL, strtol() stores the address of the first invalid character in *endptr.
 * If there were no digits at all, strtol() stores the original value of nptr in *endptr (and
 * returns 0).  In particular, if  *nptr  is  not  '\0'  but **endptr is '\0' on return, the
 * entire string is valid.
 *
 * @param nptr the string
 * @param endptr if not NULL, it will be set to the first character that doesn't belong to the
 * 	converted integer
 * @param base the base (2 - 36 inclusive; 0 = determine automatically)
 * @return the number
 */
long strtol(const char *nptr,char **endptr,int base);
llong strtoll(const char *nptr,char **endptr,int base);

/**
 * The  strtoul()  function  converts  the initial part of the string in nptr to an unsigned long
 * int value according to the given base, which must be between 2 and 36 inclusive, or be the
 * special value 0.
 *
 * The string may begin with an arbitrary amount of white space (as determined by isspace(3))
 * followed by a single  optional '+'  or  '-' sign.  If base is zero or 16, the string may
 * then include a "0x" prefix, and the number will be read in base 16; otherwise, a zero base
 * is taken as 10 (decimal) unless the next character is '0', in which case  it  is  taken  as  8
 * (octal).
 *
 * The remainder of the string is converted to an unsigned long int value in the obvious manner,
 * stopping at the first character which is not a valid digit in the given base.  (In bases above
 * 10, the letter 'A' in either  upper  or  lower  case represents 10, 'B' represents 11, and
 * so forth, with 'Z' representing 35.)
 *
 * If  endptr  is not NULL, strtoul() stores the address of the first invalid character in *endptr.
 * If there were no digits at all, strtoul() stores the original value of nptr in *endptr
 * (and returns 0).  In particular, if *nptr is not '\0'  but **endptr is '\0' on return, the
 * entire string is valid.
 *
 * @param nptr the string
 * @param endptr if not NULL, it will be set to the first character that doesn't belong to the
 * 	converted integer
 * @param base the base (2 - 36 inclusive; 0 = determine automatically)
 * @return the number
*/
ulong strtoul(const char *nptr,char **endptr,int base);
ullong strtoull(const char *nptr,char **endptr,int base);

/**
 * The strtod, strtof, and strtold functions convert the initial portion of the string
 * pointed to by nptr to double, float, and long double representation,
 * respectively. First, they decompose the input string into three parts: an initial, possibly
 * empty, sequence of white-space characters (as specified by the isspace function), a
 * subject sequence resembling a floating-point constant or representing an infinity or NaN;
 * and a final string of one or more unrecognized characters, including the terminating null
 * character of the input string. Then, they attempt to convert the subject sequence to a
 * floating-point number, and return the result.
 *
 * @param nptr the pointer to the floating-point-number
 * @param endptr will point to the end of the number (the first character that doesn't belong to it),
 * 	if not NULL
 * @return the value or 0 if conversion failed
 */
float strtof(const char *nptr,char **endptr);
double strtod(const char *nptr,char **endptr);
long double strtold(const char *nptr,char **endptr);

/**
 * The  ecvt() function converts number to a null-terminated string of ndigits digits (where
 * ndigits is reduced to a system-specific limit determined by the precision of a double), and
 * returns a pointer to the string.  The  high-order  digit  is non-zero,  unless  number  is
 * zero.  The low order digit is rounded.  The string itself does not contain a decimal point;
 * however, the position of the decimal point relative to the start of the string is stored in
 * *decpt.  A negative value for *decpt  means that the decimal point is to the left of the start
 * of the string.  If the sign of number is negative, *sign is set to a non-zero value, otherwise
 * it is set to 0.  If number is zero, it is unspecified whether *decpt is 0 or 1.
 */
char *ecvt(double number,int ndigits,int *decpt,int *sign);

#if defined(__cplusplus)
}
#endif
