/**
 * $Id: specreg.h 153 2011-04-06 16:46:17Z nasmussen $
 */

#ifndef SPECREG_H_
#define SPECREG_H_

#include "common.h"

/**
 * Description of a special-register
 */
typedef struct {
	const char *name;
	const char *desc;
} sSpecialReg;

/**
 * Explains the bits that are set in <bits> for the given register.
 *
 * @param rno the register (rA,rK,rQ,rN,rV or rU)
 * @param bits the bits that are set
 * @return the explanation (statically allocated)
 */
const char *sreg_explain(int rno,octa bits);

/**
 * Returns the description of the given special-register
 *
 * @param rno the register-number
 * @return the description or NULL
 */
const sSpecialReg *sreg_get(int rno);

/**
 * Returns the number of the special-register with given name
 *
 * @param name the name: "rA","rB",...
 * @return the number or -1
 */
int sreg_getByName(const char *name);

#endif /* SPECREG_H_ */
