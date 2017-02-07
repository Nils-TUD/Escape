/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#pragma once

#include <sys/common.h>

#define no_argument			0
#define required_argument	1
#define optional_argument	2	/* unsupported */

/**
 * Describes a long option
 */
struct option {
	const char *name;
	int has_arg;
	int *flag;	/* unsupported */
	int val;
};

/**
 * Points to the argument of an option or is NULL if there is none
 */
extern char *optarg;
/**
 * The next index in argv that is considered
 */
extern int optind;
/**
 * Whether an error message should be printed to stderr
 */
extern int opterr;
/**
 * The character that was considered in case '?' is returned
 */
extern int optopt;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Parses the given arguments for given options.
 *
 * <optstring> is used for short options ("-o") and consists of the option characters. A ':' after
 * an option character specifies that the option has an argument.
 * If an option is found, the character is returned. If it has an argument, <optarg> points to the
 * argument (either the text after the option character or the text of the next item in argv).
 *
 * <longopts> can optionally be specified for long options ("--option"). The last array element
 * needs to consist of zeros. <name> specifies the option name ("option"), <has_arg> specifies
 * whether it has an argument (*_argument) and <val> the value to return if that option is found.
 *
 * @param argc the number of args, as received in main
 * @param argv the arguments, as received in main
 * @param optstring the short options specification
 * @param longopts the long options specification
 * @param longindex if non-NULL, optind will be set to *longindex
 * @return the option or -1 if no option has been found
 */
int getopt_long(int argc,char *const argv[],const char *optstring,
	const struct option *longopts,int *longindex);
static inline int getopt(int argc,char *const argv[],const char *optstring) {
	return getopt_long(argc,argv,optstring,NULL,NULL);
}

/**
 * Converts the given string to a number, supporting the suffixes K,M, and G (lower and upper case)
 *
 * @param str the string
 * @return the number
 */
size_t getopt_tosize(const char *str);

/**
 * Checks whether the given arguments may be a kind of help-request. That means one of:
 * <prog> --help
 * <prog> -h
 * <prog> -?
 *
 * @param argc the number of arguments
 * @param argv the arguments
 * @return true if it is a help-request
 */
bool getopt_ishelp(int argc,char **argv);

#if defined(__cplusplus)
}
#endif
