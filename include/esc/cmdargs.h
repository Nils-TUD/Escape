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

#ifndef CMDARGS_H_
#define CMDARGS_H_

#include <esc/common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CA_NO_DASHES				1		/* disallow '-' or '--' for arguments */
#define CA_REQ_EQ					2		/* require '=' for arguments */
#define CA_NO_EQ					4		/* disallow '=' for arguments */
#define CA_NO_FREE					8		/* throw exception if free arguments are found */
#define CA_MAX1_FREE				16		/* max. 1 free argument */

#define CA_ERR_REQUIRED_MISSING		-1
#define CA_ERR_EQ_REQUIRED			-2
#define CA_ERR_EQ_IN_FLAGARG		-3
#define CA_ERR_EQ_DISALLOWED		-4
#define CA_ERR_FREE_DISALLOWED		-5
#define CA_ERR_MAX1_FREE			-6
#define CA_ERR_NO_FREE				-7

const char *ca_error(s32 err);

bool ca_hasHelp(void);

const char **ca_getfree(void);

s32 ca_parse(int argcnt,const char **args,u16 aflags,const char *fmt,...);

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
bool isHelpCmd(int argc,char **argv);

#ifdef __cplusplus
}
#endif

#endif /* CMDARGS_H_ */
