/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef ASSERT_H_
#define ASSERT_H_

#if IN_KERNEL
#	include <util.h>
#else
#	include <esc/debug.h>
#endif

#ifndef NDEBUG

#	define assert(cond) vassert(cond,"")

#	if IN_KERNEL
#		define vassert(cond,errorMsg,...) do { if(!(cond)) { \
			panic("Assert '" #cond "' failed at %s, %s() line %d: " errorMsg,__FILE__,__FUNCTION__,\
				__LINE__,## __VA_ARGS__); \
		} } while(0);
#	else
#		define vassert(cond,errorMsg,...) do { if(!(cond)) { \
			debugf("Assert '" #cond "' failed at %s, %s() line %d: " errorMsg,__FILE__,__FUNCTION__,\
				__LINE__,## __VA_ARGS__); \
				exit(1); \
		} } while(0);
#	endif

#else

#	define assert(cond)
#	define vassert(cond,errorMsg,...)

#endif

#endif /* ASSERT_H_ */
