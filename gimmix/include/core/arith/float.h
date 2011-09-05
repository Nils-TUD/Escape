/**
 * $Id: float.h 209 2011-05-15 09:37:13Z nasmussen $
 */

#ifndef FLOATARITH_H_
#define FLOATARITH_H_

#include "common.h"

// round modes
#define ROUND_OFF		1		// code in rA: 01 - round towards 0
#define ROUND_UP		2		// code in rA: 10 - round towards +inf
#define ROUND_DOWN		3		// code in rA: 11 - round towards -inf
#define ROUND_NEAR		4		// code in rA: 00 - and to even in case of ties

// results of fl_compareWithEps
#define ECMP_NOT_EQUAL	0
#define ECMP_EQUAL		1
#define ECMP_INVALID	2

// results of fl_compare
#define CMP_LESS		(-(octa)1)
#define CMP_EQUAL		0
#define CMP_GREATER		1
#define CMP_UNORDERED	2

// types of floats
typedef enum {
	ZERO, NUM, INF, NAN
} eFtype;

// an unpacked, normalized float
typedef struct {
	eFtype type;
	char s;			// sign: '-' or '+'
	int e;			// exponent - 1
	octa f;			// fraction << 2 with bit 2^54 set
} sFloat;

/**
 * Converts the given 32-bit float to a 64-bit float
 *
 * @param z the short float
 * @param ex the arithmetic exceptions that have been set
 * @return the 64-bit float
 */
octa fl_shortToFloat(tetra z,unsigned *ex);

/**
 * Converts the given 64-bit float to a 32-bit float
 *
 * @param x the 64-bit float
 * @param ex the arithmetic exceptions that have been set
 * @return the short float
 */
tetra fl_floatToShort(octa x,unsigned *ex);

/**
 * Multiplies y with z
 *
 * @param y floating-point-number
 * @param z floating-point-number
 * @param ex the arithmetic exceptions that have been set
 * @return the result
 */
octa fl_mult(octa y,octa z,unsigned *ex);

/**
 * Divides y by z
 *
 * @param y floating-point-number
 * @param z floating-point-number
 * @param ex the arithmetic exceptions that have been set
 * @return the result
 */
octa fl_divide(octa y,octa z,unsigned *ex);

/**
 * Adds y to z
 *
 * @param y floating-point-number
 * @param z floating-point-number
 * @param ex the arithmetic exceptions that have been set
 * @return the result
 */
octa fl_add(octa y,octa z,unsigned *ex);

/**
 * Adds y to (z ^ (1 << 63))
 *
 * @param y floating-point-number
 * @param z floating-point-number
 * @param ex the arithmetic exceptions that have been set
 * @return the result
 */
octa fl_sub(octa y,octa z,unsigned *ex);

/**
 * Compares y to z with respect to e. It will be ECMP_INVALID if e is invalid or if y or z is NaN.
 * It will be ECMP_NOT_EQUAL if y < z or y > z or y ~ z when not comparing for similarity. Otherwise
 * it will be ECMP_EQUAL.
 *
 * @param y floating-point-number
 * @param z floating-point-number
 * @param e floating-point-number
 * @param similarity 1 if we should compare for similarity, i.e. if it has to be y ~~ z for
 *   ECMP_EQUAL
 * @return the result (ECMP_*)
 */
octa fl_compareWithEps(octa y,octa z,octa e,int similarity);

/**
 * Compares y to z. The result is CMP_LESS if y < z, CMP_GREATER if y > z, CMP_EQUAL if y = z or
 * CMP_UNORDERED if they can't be compared, i.e. at least one of them is NaN.
 *
 * @param y floating-point-number
 * @param z floating-point-number
 * @return the result (CMP_*)
 */
octa fl_compare(octa y,octa z);

/**
 * Interizes the given float. That means, it will interpret z as a floating-point number, cut the
 * fraction and pack it again to a floating-point number, depending on the given rounding-mode.
 *
 * @param z the float
 * @param rounding the rounding-mode to use (0 = mode in rA)
 * @param ex the arithmetic exceptions that have been set
 * @return the float with zero fraction
 */
octa fl_interize(octa z,int rounding,unsigned *ex);

/**
 * Fixes the given float. That means, it will interpret z as a floating-point number and translate
 * it to the next near integer, depending on the given rounding-mode.
 *
 * @param z the float
 * @param rounding the rounding-mode to use (0 = mode in rA)
 * @param ex the arithmetic exceptions that have been set
 * @return the integer
 */
octa fl_fixit(octa z,int rounding,unsigned *ex);

/**
 * Floats the given integer. That means, it will in interpret z as an integer and translate it to
 * a float.
 *
 * @param z the float
 * @param rounding the rounding-mode to use (0 = mode in rA)
 * @param unSigned whether z should be treaten as an unsigned integer
 * @param shortPrec whether the result should be in short-float-range
 * @param ex the arithmetic exceptions that have been set
 * @return the float
 */
octa fl_floatit(octa z,int rounding,bool unSigned,bool shortPrec,unsigned *ex);

/**
 * Calculates the square-root of z in given rounding-mode
 *
 * @param z the float
 * @param rounding the rounding-mode to use (0 = mode in rA)
 * @param ex the arithmetic exceptions that have been set
 * @return the result
 */
octa fl_sqrt(octa z,int rounding,unsigned *ex);

/**
 * Calculates the floating-point remainder of y and z. That means n in y - n * z, so that n is the
 * nearest integer to y/z, and n is an even integer in case of ties.
 *
 * @param y floating-point-number
 * @param z floating-point-number
 * @param delta the decrease in exponent that is acceptable for incomplete results (e.g. 2500 =>
 *  computable in one step)
 * @param ex the arithmetic exceptions that have been set
 * @return the result
 */
octa fl_remstep(octa y,octa z,int delta,unsigned *ex);

#endif /* FLOATARITH_H_ */
