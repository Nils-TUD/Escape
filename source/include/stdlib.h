/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <esc/common.h>
#include <esc/proc.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
using namespace std;
#endif

/* max rand-number */
#define RAND_MAX 32767

/* results of div, ldiv and lldiv */
typedef struct {
	int quot;
	int rem;
} div_t;
typedef struct {
	long quot;
	long rem;
} ldiv_t;
typedef struct {
	llong quot;
	llong rem;
} lldiv_t;

/* function that compares <a> and <b> and returns:
 * 	<a> <  <b>: negative value
 *  <a> == <b>: 0
 *  <a> >  <b>: positive value
 *
 * @param a the first operand
 * @param b the second operand
 * @return the compare-result
 */
typedef int (*fCompare)(const void *a,const void *b);

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
 * @param nptr the string
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
 * Rand will generate a random number between 0 and 'RAND_MAX' (at least 32767).
 *
 * @return the random number
 */
int rand(void);

/**
 * Srand seeds the random number generation function rand so it does not produce the same
 * sequence of numbers.
 */
void srand(uint seed);

/**
 * Allocates space for <nobj> elements, each <size> big, on the heap and memset's the area to 0.
 * If there is not enough memory the function returns NULL.
 *
 * @param num the number of elements
 * @param size the size of each element
 * @return the address of the memory or NULL
 */
void *calloc(size_t nobj,size_t size);

/**
 * Frees the area starting at <addr>.
 */
void free(void *addr);

/**
 * Allocates <size> bytes on the heap and returns the pointer to the beginning of
 * the allocated memory. If there is not enough memory the function returns NULL.
 *
 * @param size the number of bytes to allocate
 * @return the address of the memory or NULL
 */
void *malloc(size_t size);

/**
 * Reallocates the area at given address to the given size. That means either your data will
 * be copied to a different address or your area will be resized.
 *
 * @param addr the address of your area
 * @param size the number of bytes your area should be resized to
 * @return the address (may be different) of your area or NULL if there is not enough mem
 */
void *realloc(void *addr,size_t size);

/**
 * Checks whether <ptr> resides on the heap.
 *
 * @param ptr the pointer to your object
 * @return true if <ptr> is on the heap
 */
bool isOnHeap(const void *ptr);

/**
 * Note that the heap does increase the data-pages of the process as soon as it's required and
 * does not decrease them. So the free-space may increase during runtime!
 *
 * @return the free space on the heap
 */
size_t heapspace(void);

#if DEBUGGING
/**
 * Prints the heap
 */
void printheap(void);
#endif

/**
 * Aborts the process
 */
void abort(void);

/**
 * Displays an error-message according to given format and arguments and appends ': <errmsg>' if
 * errno is != 0. After that exit(EXIT_FAILURE) is called.
 *
 * @param fmt the error-message-format
 */
void error(const char *fmt,...) A_NORETURN;

/**
 * The atexit function registers the function pointed to by func, to be called without
 * arguments at normal program termination.
 *
 * @param func the function
 * @return zero if the registration succeeds, nonzero if it fails
 */
int atexit(void (*func)(void *));

/**
 * Exit causes the program to end and supplies a status code to the calling environment.
 *
 * @param status the status-code
 */
void exit(int status);

/**
 * The _Exit function causes normal program termination to occur and control to be
 * returned to the host environment. No functions registered by the atexit function or
 * signal handlers registered by the signal function are called. The status returned to the
 * host environment is determined in the same way as for the exit function (7.20.4.3).
 * Whether open streams with unwritten buffered data are flushed, open streams are closed,
 * or temporary files are removed is implementation-defined.
 *
 * @param status the status-code
 */
void _Exit(int status);

/**
 * Fetches the value of the given environment-variable
 *
 * @param value the buffer to write the value to
 * @param valSize the size of the buffer
 * @param name the environment-variable-name
 * @return 0 if successfull
 */
A_CHECKRET static inline int getenvto(char *value,size_t valSize,const char *name) {
	return syscall3(SYSCALL_GETENVTO,(ulong)value,valSize,(ulong)name);
}

/**
 * Returns the value of the given environment-variable
 *
 * @param name the environment-variable-name
 * @return the value (statically allocated) or NULL
 */
char *getenv(const char *name) A_CHECKRET;

/**
 * Fetches the env-variable-name with given index
 *
 * @param name the buffer to write the name to
 * @param nameSize the size of the buffer
 * @param index the index
 * @return 0 if successfull
 */
static inline int getenvito(char *name,size_t nameSize,size_t index) {
	return syscall3(SYSCALL_GETENVITO,(ulong)name,nameSize,index);
}

/**
 * Returns the env-variable-name with given index
 *
 * @param index the index
 * @return the value (statically allocated) or NULL
 */
char *getenvi(size_t index) A_CHECKRET;

/**
 * Sets the environment-variable <name> to <value>.
 *
 * @param name the name
 * @param value the value
 * @return 0 on success
 */
static inline int setenv(const char *name,const char *value) {
	return syscall2(SYSCALL_SETENV,(ulong)name,(ulong)value);
}

/**
 * The system function is used to issue a command. Execution of your program will not
 * continue until the command has completed.
 *
 * @param cmd string containing the system command to be executed.
 * @return The value returned when the argument passed is not NULL, depends on the running
 * 	environment specifications. In many systems, 0 is used to indicate that the command was
 * 	successfully executed and other values to indicate some sort of error.
 * 	When the argument passed is NULL, the function returns a nonzero value if the command
 * 	processor is available, and zero otherwise.
 */
int system(const char *cmd);

/**
 * Searches the given key in the array pointed by base that is formed by num elements,
 * each of a size of size bytes, and returns a void* pointer to the first entry in the table
 * that matches the search key.
 *
 * To perform the search, the function compares the elements to the key by calling the function
 * comparator specified as the last argument.
 *
 * Because this function performs a binary search, the values in the base array are expected to be
 * already sorted in ascending order, with the same criteria used by comparator.
 *
 * @param key the key to search for
 * @param base the array to search
 * @param num the number of elements in the array
 * @param size the size of each element
 * @param cmp the compare-function
 * @return a pointer to an entry in the array that matches the search key or NULL if not found
 */
void *bsearch(const void *key,const void *base,size_t num,size_t size,fCompare cmp);

/**
 * Sorts the num elements of the array pointed by base, each element size bytes long, using the
 * comparator function to determine the order.
 * The sorting algorithm used by this function compares pairs of values by calling the specified
 * comparator function with two pointers to elements of the array.
 * The function does not return any value, but modifies the content of the array pointed by base
 * reordering its elements to the newly sorted order.
 *
 * @param base the array to sort
 * @param num the number of elements
 * @param size the size of each element
 * @param cmp the compare-function
 */
void qsort(void *base,size_t num,size_t size,fCompare cmp);

/**
 * @param j the number
 * @return absolute value of <j>
 */
int abs(int j);
long labs(long j);
llong llabs(llong j);

/**
 * Returns the integral quotient and remainder of the division of numerator by denominator as a
 * structure of type div_t, which has two members: quot and rem.
 *
 * @param numerator the numerator
 * @param denominator the denominator
 * @return quotient and remainder
 */
div_t div(int numer,int denom);
ldiv_t ldiv(long numer,long denom);
lldiv_t lldiv(llong numer,llong denom);

#ifdef __cplusplus
}
#endif
