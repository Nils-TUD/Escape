/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef STDDEF_H_
#define STDDEF_H_

#include <types.h>

typedef u32 ptrdiff_t;
typedef u32 size_t;

#define offsetof(type,field) ((size_t)(&((type *)0)->field))

#define NULL (void*)0

#endif /* STDDEF_H_ */
