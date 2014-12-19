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

#include <sys/common.h>
#include <sys/cmdargs.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <usergroup/user.h>
#include <usergroup/group.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sUser *userList;
static sGroup *groupList;

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [<user>][:[<group>]] <file>...\n",name);
	exit(EXIT_FAILURE);
}

static const char *parseName(const char *s,bool *numeric) {
	static char tmp[64];
	*numeric = true;
	size_t i;
	for(i = 0; i < sizeof(tmp) - 1; ++i) {
		if(*s == ':' || *s == '\0')
			break;
		if(!isdigit(*s))
			*numeric = false;
		tmp[i] = *s++;
	}
	tmp[i] = '\0';
	return tmp;
}

static bool parseUserGroup(const char *spec,uid_t *uid,gid_t *gid) {
	char *s = (char*)spec;
	if(*s != ':') {
		bool numeric = false;
		const char *uname = parseName(s,&numeric);

		sUser *u;
		if(numeric) {
			uid_t uid = strtoul(uname,NULL,10);
			u = user_getById(userList,uid);
		}
		else
			u = user_getByName(userList,uname);
		if(!u) {
			fprintf(stderr,"Unable to find user '%s'\n",uname);
			return false;
		}

		*uid = u->uid;
		s += strlen(uname);
		if(*s == ':')
			s++;
	}
	else
		s++;

	if(*s == '\0')
		return true;

	bool numeric = false;
	const char *gname = parseName(s,&numeric);

	sGroup *g;
	if(numeric) {
		gid_t gid = strtoul(gname,NULL,10);
		g = group_getById(groupList,gid);
	}
	else
		g = group_getByName(groupList,gname);
	if(!g) {
		fprintf(stderr,"Unable to find group '%s'\n",gname);
		return false;
	}

	*gid = g->gid;

	s += strlen(gname);
	if(*s != '\0') {
		fprintf(stderr,"Invalid user/group spec '%s'\n",spec);
		return false;
	}
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

	userList = user_parseFromFile(USERS_PATH,NULL);
	if(!userList)
		printe("Warning: unable to parse users from file");
	groupList = group_parseFromFile(GROUPS_PATH,NULL);
	if(!groupList)
		printe("Unable to parse groups from file");

	if(!parseUserGroup(spec,&uid,&gid))
		exit(EXIT_FAILURE);

	args = ca_getFree();
	while(*args) {
		if(chown(*args,uid,gid) < 0)
			printe("Unable to set owner/group of '%s' to %u/%u",*args,uid,gid);
		args++;
	}
	return EXIT_SUCCESS;
}
