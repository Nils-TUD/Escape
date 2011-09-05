/**
 * $Id: int.h 154 2011-04-06 19:07:14Z nasmussen $
 */

#ifndef INTARITH_H_
#define INTARITH_H_

#include "common.h"

/**
 * Performs an unsigned 128-bit multiplication of x and y. The lower 64-bit of the result will be
 * returned, the upper 64-bit are stored in aux.
 *
 * @param x the first operand
 * @param y the second operand
 * @param aux the upper 64-bit of the result
 * @return the lower 64-bit of the result
 */
octa int_umult(octa x,octa y,octa *aux);

/**
 * Performs a signed 128-bit multiplication of x and y. The lower 64-bit of the result will be
 * returned, the upper 64-bit are stored in aux.
 *
 * @param x the first operand
 * @param y the second operand
 * @param aux the upper 64-bit of the result
 * @return the lower 64-bit of the result
 */
octa int_smult(octa x,octa y,octa *aux);

/**
 * Performs an unsigned 128-bit division of ((x << 64) | y) / z.
 * It will put the remainder into rem.
 *
 * @param x the upper 64-bit of the dividend
 * @param y the lower 64-bit of the dividend
 * @param z the divisor
 * @param rem the remainder
 * @return the result
 */
octa int_udiv(octa x,octa y,octa z,octa *rem);

/**
 * Performs a signed 64-bit division of y / z.
 *
 * @param y the dividend
 * @param z the divisor
 * @param rem the remainder
 * @return the result
 */
octa int_sdiv(octa y,octa z,octa *rem);

/**
 * Performs a left-shift of y by z places
 *
 * @param y the integer
 * @param z the number of places to shift
 * @return the result
 */
octa int_shiftLeft(octa y,octa z);

/**
 * Performs an arithmetic right-shift of y by z places
 *
 * @param y the integer
 * @param z the number of places to shift
 * @return the result
 */
octa int_shiftRightArith(octa y,octa z);

/**
 * Performs a logical right-shift of y by z places
 *
 * @param y the integer
 * @param z the number of places to shift
 * @return the result
 */
octa int_shiftRightLog(octa y,octa z);

#endif /* INTARITH_H_ */
