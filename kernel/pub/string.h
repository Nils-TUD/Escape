/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef STRING_H_
#define STRING_H_

#include "common.h"

/**
 * The atoi() function converts str into an integer, and returns that integer. str should start
 * with whitespace or some sort of number, and atoi() will stop reading from str as soon as
 * a non-numerical character has been read.
 *
 * @param str the string
 * @return the integer
 */
s32 atoi(cstring str);

/**
 * Converts the given signed integer to a string
 *
 * @param target the target-string (needs max. 12 elements)
 * @param n the integer
 */
void itoa(string target,s32 n);

/**
 * The memchr() function looks for the first occurrence of c within count characters in
 * the array pointed to by buffer.
 *
 * @param buffer the buffer to search in
 * @param c the element to search for
 * @param count the number of characters to loop through
 * @return NULL if not found or the pointer to the found element
 */
void *memchr(const void *buffer,s32 c,u32 count);

/**
 * Copies <len> words from <src> to <dest>
 *
 * @param dest the destination-address
 * @param src the source-address
 * @param len the number of words to copy
 */
void *memcpy(void *dest,const void *src,u32 len);

/**
 * Compares <count> words of <str1> and <str2> and returns wether they are equal
 *
 * @param str1 the first data
 * @param str2 the second data
 * @param count the number of words
 * @return -1 if <str1> lt <str2>, 0 if equal and 1 if <str1> gt <str2>
 */
s32 memcmp(const void *str1,const void *str2,u32 count);

/**
 * Sets all bytes in memory beginning at <addr> and ending at <addr> + <count>
 * to <value>.
 *
 * @param addr the starting address
 * @param value the value to set
 * @param count the number of bytes
 */
void memset(void *addr,u32 value,u32 count);

/**
 * The strcpy() function copies characters in the string from to the string to, including the
 * null termination.
 * Note that strcpy() does not perform bounds checking, and thus risks overrunning from or to.
 * For a similar (and safer) function that includes bounds checking, see strncpy().
 *
 * @param to the target
 * @param from the source
 * @return to
 */
string strcpy(string to,cstring from);

/**
 * The strncpy() function copies at most count characters of from to the string to.
 * If from has less than count characters, the remainder is padded with '\0' characters.
 *
 * @param to the target
 * @param from the source
 * @param count the number of chars to copy
 * @return the target string
 */
string strncpy(string to,cstring from,u32 count);

/**
 * The strcat() function concatenates str2 onto the end of str1, and returns str1.
 *
 * @param str1 the string to append to
 * @param str2 the string to append
 * @return pointer to str1
 */
string strcat(string str1,cstring str2);

/**
 * The function strncat() concatenates at most count characters of str2 onto str1,
 * adding a null termination
 *
 * @param str1 the string to append to
 * @param str2 the string to append
 * @param count the number of elements to concatenate to str1
 * @return resulting string
 */
string strncat(string str1,cstring str2,u32 count);

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
s32 strcmp(cstring str1,cstring str2);

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
s32 strncmp(cstring str1,cstring str2,u32 count);

/**
 * The function strchr() returns a pointer to the first occurence of ch in str, or NULL
 * if ch is not found.
 *
 * @param str the string
 * @param ch the character
 * @return NULL if not found or the pointer to the found character
 */
string strchr(cstring str,s32 ch);

/**
 * Returns the index of the first occurrences of ch in str or strlen(str) if ch is not found.
 *
 * @param str the string
 * @param ch the character
 * @return the index of the character or strlen(str) if not found
 */
s32 strchri(cstring str,s32 ch);

/**
 * @param str the string to search
 * @param ch the character to search for
 * @return a pointer to the last occurrence of ch in str, or NULL if no match is found
 */
string strrchr(cstring str,s32 ch);

/**
 * The function strstr() returns a pointer to the first occurrence of str2 in str1, or NULL
 * if no match is found. If the length of str2 is zero, then strstr () will simply return str1.
 *
 * @param str1 the string to search
 * @param str2 the substring to search for
 * @return the pointer to the match or NULL
 */
string strstr(cstring str1,cstring str2);

/**
 * @param str1 the string to search in
 * @param str2 the character list
 * @return the index of the first character in str1 that matches any of the characters in str2.
 */
u32 strcspn(cstring str1,cstring str2);

/**
 * Cuts out the first count characters in the given string. That means all characters behind
 * will be moved count chars back.
 *
 * @param str the string
 * @param count the number of chars to remove
 * @return the string
 */
string strcut(string str,u32 count);

/**
 * @param str the string
 * @return the length of str (determined by the number of characters before null termination).
 */
u32 strlen(string str);

/**
 * @param ch the char
 * @return the lowercase version of the character ch.
 */
s32 tolower(s32 ch);

/**
 * @param ch the char
 * @return the uppercase version of the character ch.
 */
s32 toupper(s32 ch);

/**
 * @param c the character
 * @return non-zero if its argument is a numeric digit or a letter of the alphabet.
 * 	Otherwise, zero is returned.
 */
bool isalnum(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is a letter of the alphabet. Otherwise, zero is returned.
 */
bool isalpha(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is a digit between 0 and 9. Otherwise, zero is returned.
 */
bool isdigit(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is a lowercase letter. Otherwise, zero is returned.
 */
bool islower(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is some sort of space (i.e. single space, tab,
 * 	vertical tab, form feed, carriage return, or newline). Otherwise, zero is returned.
 */
bool isspace(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is an uppercase letter. Otherwise, zero is returned.
 */
bool isupper(s32 c);

/*
atol	converts a string to a long
memmove	moves one buffer to another
strtod	converts a string to a double
strtol	converts a string to a long
strtoul	converts a string to an unsigned long
*/

#endif /* STRING_H_ */
