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

#include <esc/common.h>
#include <esc/cmdargs.h>
#include <esc/proc.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [<user>][:[<group>]] <file>...\n",name);
	exit(EXIT_FAILURE);
}

static bool parseUserGroup(const char *spec,uid_t *uid,gid_t *gid) {
	char *s = (char*)spec;
	if(*s != ':') {
		*uid = strtoul(s,&s,0);
		if(*s == '\0')
			return true;
		if(*s != ':')
			return false;
		s++;
	}
	else
		s++;
	if(*s == '\0')
		return true;
	*gid = strtoul(s,&s,0);
	if(*s != '\0')
		return false;
	return true;
}

int main(int argc,const char **argv) {
	char *spec = NULL;
	uid_t uid = -1;
	gid_t gid = -1;
	const char **args;

	int res = ca_parse(argc,argv,0,"=s*",&spec);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	if(!parseUserGroup(spec,&uid,&gid))
		error("Invalid specification: '%s'",spec);
	args = ca_getFree();
	while(*args) {
		if(chown(*args,uid,gid) < 0)
			printe("Unable to set owner/group of '%s' to %u/%u",*args,uid,gid);
		args++;
	}
	return EXIT_SUCCESS;
}
