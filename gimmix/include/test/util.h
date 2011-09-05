/**
 * $Id: util.h 217 2011-06-03 18:11:57Z nasmussen $
 */

#ifndef UTIL_H_
#define UTIL_H_

#include "common.h"

#define ROUND_COUNT		4

typedef struct {
	unsigned mmix;
	double (*cFunc)(double);
	unsigned x86;
	const char *name;
} sRounding;

const sRounding *getRoundings(void);
void setFPUCW(unsigned int mode);
bool isInf(octa x);
bool isZero(octa x);
bool isNaN(octa x);
bool isSignalingNaN(octa x);
octa setSign(octa x,bool set);
uDouble correctResForNaNOps(uDouble res,uDouble y,uDouble z);
char *strtoupper(char *s);

#endif /* UTIL_H_ */
