/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include <esc/common.h>
#include <esc/setjmp.h>
#include <esc/heap.h>

/**
 * This module tries to emulate exceptions known from c++ or java. You can use it (nearly) as
 * you're used to it except that you have to note the following restrictions:
 * 	- You can't use THROW in catch- and finally-blocks (just RETHROW)
 * 	- You can't use TRY-CATCH in catch-blocks
 *  - Atm it's not thread-safe!
 *  - Of course, you have to take care not to manipulate the control-flow in try/catch/finally.
 *    That means, you can't use break, goto etc., since otherwise FINALLY or ENDCATCH
 *    wouldn't be executed
 *
 * An example would be:
 * #include <exceptions/io.h>
 *
 * TRY {
 * 	THROW(IOException,ERR_EOF);
 * }
 * CATCH(IOException,e) {
 * 	printf("Catched exception thrown at %s:%d\n",e->file,e->line);
 * }
 * FINALLY {
 * 	printf("Finally called\n");
 * }
 * ENDCATCH
 */

#define TRY							{ sException *__ex = NULL; \
										if(setjmp(ex_push()) != 0) \
											__ex = __exPtr; \
										else {

#define CATCH(name,obj)				} { s##name *(obj) = (s##name*)__ex; \
										if((obj) && (obj)->id == (ID_##name) && ((obj)->handled = 1))

#define FINALLY						} {

#define ENDCATCH					} \
									if(__ex) { \
										if(!__ex->handled) \
											ex_unwind(__ex); \
										else \
											free(__ex); \
									} \
									ex_pop(); \
								}

#define THROW(name,...)				ex_unwind((__exPtr = (sException*)\
										ex_create##name(ID_##name,__LINE__,__FILE__,## __VA_ARGS__)))
#define RETHROW(exObj)				(__ex = (sException*)(exObj))->handled = 0

typedef const char *(*fToString)(void *e);

typedef struct {
	s32 handled;
	s32 id;
	s32 line;
	const char *file;
	fToString toString;
} sException;

extern sException *__exPtr;

sJumpEnv *ex_push(void);
void ex_pop(void);
void ex_unwind(void *exObj);

#endif /* EXCEPTION_H_ */
