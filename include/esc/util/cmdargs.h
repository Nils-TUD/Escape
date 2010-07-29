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

#ifndef ESCCMDARGS_H_
#define ESCCMDARGS_H_

#include <esc/common.h>
#include <esc/util/iterator.h>
#include <esc/sllist.h>
#include <stdarg.h>

#define CA_NO_DASHES	1		/* disallow '-' or '--' for arguments */
#define CA_REQ_EQ		2		/* require '=' for arguments */
#define CA_NO_EQ		4		/* disallow '=' for arguments */
#define CA_NO_FREE		8		/* throw exception if free arguments are found */
#define CA_MAX1_FREE	16		/* max. 1 free argument */

/**
 * A example of this 'class' would be:
 *
 * char *s = NULL;
 * s32 d = 0;
 * u32 k = 0,flag = 0;
 * sCmdArgs *a;
 * TRY {
 *   a = cmdargs_create(argc,argv,0);
 *   a->parse(a,"=s a1=d a2=k flag",&s,&a1,&a2,&flag);
 *   if(a->isHelp)
 *     usage(argv[0]);
 * }
 * CATCH(CmdArgsException,e) {
 *   cerr->writef(cerr,"Invalid arguments: %s",e->toString(e));
 *   usage(argv[0]);
 * }
 * ENDCATCH
 */

typedef struct sCmdArgs sCmdArgs;

struct sCmdArgs {
/* private: */
	int _argc;
	const char **_argv;
	sSLList *_freeArgs;
	u16 _flags;

/* public: */
	/**
	 * True if the command-line-arguments contain a help-request
	 */
	u16 isHelp;

	/**
	 * Parses the command-line-arguments according to given format. It sets the given variable
	 * parameters depending on the format. The format works like the following:
	 * The first chars are the name of the argument, after that follows optionally a '=' which
	 * tells the parser that the argument has a value. Behind it the type of the value follows.
	 * The type may be:
	 * 	's':			a string, expects a char** as argument
	 * 	'd' or 'i':		a signed integer, expects a s32* as argument
	 * 	'u':			a unsigned integer to base 10, expects a u32* as argument
	 * 	'x' or 'X':		a unsigned integer to base 16, expects a u32* as argument
	 * 	'c':			a character, expects a char* as argument
	 * 	'k':			a unsigned integer with optional suffix K,M or G. expects a u32* as argument
	 * After the type may follow a '*' to tell the parser that the argument is required. If the
	 * argument has no value a bool* is expected as argument. The arguments have to be separated
	 * by spaces.
	 *
	 * An example is:
	 * bool flag;
	 * char *s;
	 * s32 d;
	 * args->parse(args,"flag a=s arg2=d*",&flag,&s,&d);
	 *
	 * There are some special things to know:
	 * 	- If an required argument is missing, a argument-value is missing, free arguments are given
	 *    but not allowed, or similar, a CmdArgsException is thrown.
	 * 	- The name of an argument may be empty. This means implicit that its an required argument
	 *    which has just a value. Those arguments are expected in the given order. So if you
	 *    specify an empty argument as the first, it has to be the first in the program-arguments
	 *    (after the program-name of course).
	 * Additionally you can manipulate the behaviour by flags when creating the instance.
	 *
	 * Note that you have to call args->destroy(args) when you're done!
	 *
	 * @param a the args-instance
	 * @param fmt the format
	 */
	void (*parse)(sCmdArgs *a,const char *fmt,...);

	/**
	 * Returns an iterator (allocated on the stack, no need to free anything!). You can iterator
	 * over the free arguments (call parse() first!) like the following:
	 * sIterator it = args->getFreeArgs(args);
	 * while(it.hasNext(&it)) {
	 *   const char *arg = (const char*)it.next(&it);
	 *   // do something
	 * }
	 *
	 * @param a the args-instance
	 * @return the iterator
	 */
	sIterator (*getFreeArgs)(sCmdArgs *a);

	/**
	 * Convenience-function which returns just the first argument or NULL if not present
	 *
	 * @param a the args-instance
	 * @return the first argument
	 */
	const char *(*getFirstFree)(sCmdArgs *a);

	/**
	 * Destroys this instance
	 *
	 * @param a the args-instance
	 */
	void (*destroy)(sCmdArgs *a);
};

/**
 * Creates an command-line-arguments-instance
 *
 * @param argc the number of arguments
 * @param argv the arguments
 * @param flags flags to manipulate the behaviour (see CA_*)
 * @return the instance
 */
sCmdArgs *cmdargs_create(int argc,const char **argv,u16 flags);

#endif /* ESCCMDARGS_H_ */
