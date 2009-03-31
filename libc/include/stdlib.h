/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef STDLIB_H_
#define STDLIB_H_

#include <esc/heap.h>
#include <esc/proc.h>
#include <types.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

/* results of div and ldiv */
typedef struct {
	int quot;
	int rem;
} div_t;
typedef struct {
	long quot;
	long rem;
} ldiv_t;

/* max rand-number */
#define RAND_MAX 0xFFFFFFFF

/* exit-codes */
#define EXIT_FAILURE	1
#define EXIT_SUCCESS	0

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
 * The atoi() function converts str into an integer, and returns that integer. str should start
 * with whitespace or some sort of number, and atoi() will stop reading from str as soon as
 * a non-numerical character has been read.
 *
 * @param str the string
 * @return the integer
 */
extern int atoi(const char *str);

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
void srand(unsigned int seed);

/**
 * Allocates space for <nobj> elements, each <size> big, on the heap and memset's the area to 0.
 * If there is not enough memory the function returns NULL.
 *
 * @param num the number of elements
 * @param size the size of each element
 * @return the address of the memory or NULL
 */
extern void *calloc(size_t nobj,size_t size);

/**
 * Frees the area starting at <addr>.
 */
extern void free(void *addr);

/**
 * Allocates <size> bytes on the heap and returns the pointer to the beginning of
 * the allocated memory. If there is not enough memory the function returns NULL.
 *
 * @param size the number of bytes to allocate
 * @return the address of the memory or NULL
 */
extern void *malloc(size_t size);

/**
 * Reallocates the area at given address to the given size. That means either your data will
 * be copied to a different address or your area will be resized.
 *
 * @param addr the address of your area
 * @param size the number of bytes your area should be resized to
 * @return the address (may be different) of your area or NULL if there is not enough mem
 */
extern void *realloc(void *addr,size_t size);

/**
 * Aborts the process
 */
void abort(void);

/**
 * Exit causes the program to end and supplies a status code to the calling environment.
 *
 * @param status the status-code
 */
extern void exit(int status);

/**
 * Get an environment variable
 *
 * @param name the variable-name
 * @return the variable-value or NULL if there is no match.
 */
char *getenv(const char *name);

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
 * @param n the number
 * @return absolute value of <n>
 */
int abs(int n);

/**
 * @param n the number
 * @return absolute value of <n>
 */
long int labs(long int n);

/**
 * Returns the integral quotient and remainder of the division of numerator by denominator as a
 * structure of type div_t, which has two members: quot and rem.
 *
 * @param numerator the numerator
 * @param denominator the denominator
 * @return quotient and remainder
 */
div_t div(int numerator,int denominator);

/**
 * Returns the integral quotient and remainder of the division of numerator by denominator as a
 * structure of type ldiv_t, which has two members: quot and rem.
 *
 * @param numerator the numerator
 * @param denominator the denominator
 * @return quotient and remainder
 */
ldiv_t ldiv(long numerator,long denominator);

#endif /* STDLIB_H_ */
