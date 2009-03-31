/**
 * @version		$Id: common.h 77 2008-11-22 22:27:35Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <types.h>
#include <stddef.h>

/* file-number (in global file table) */
typedef s32 tFileNo;

#ifndef DEBUGGING
#define DEBUGGING 1
#endif

/* debugging */
#define DBG_PGCLONEPD(s)
#define DBG_KMALLOC(s)

#endif /*COMMON_H_*/
