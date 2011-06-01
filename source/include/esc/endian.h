/**
 * $Id$
 */

#ifndef ENDIAN_H_
#define ENDIAN_H_

#include <esc/common.h>

#ifdef __i386__
#include <arch/i586/endian.h>
#endif
#ifdef __eco32__
#include <arch/eco32/endian.h>
#endif

#endif /* ENDIAN_H_ */
