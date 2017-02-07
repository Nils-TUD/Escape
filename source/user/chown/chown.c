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
#include <sys/proc.h>
#include <sys/stat.h>
#include <usergroup/usergroup.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

		int userId;
		if(numeric) {
			userId = strtoul(uname,NULL,10);
			if(!usergroup_idToName(USERS_PATH,userId))
				userId = -1;
		}
		else
			userId = usergroup_nameToId(USERS_PATH,uname);
		if(userId < 0) {
			fprintf(stderr,"Unable to find user '%s'\n",uname);
			return false;
		}

		*uid = userId;
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


	int groupId;
	if(numeric) {
		groupId = strtoul(gname,NULL,10);
		if(!usergroup_idToName(GROUPS_PATH,groupId))
			groupId = -1;
	}
	else
		groupId = usergroup_nameToId(GROUPS_PATH,gname);
	if(groupId < 0) {
		fprintf(stderr,"Unable to find group '%s'\n",gname);
		return false;
	}

	*gid = groupId;

	s += strlen(gname);
	if(*s != '\0') {
		fprintf(stderr,"Invalid user/group spec '%s'\n",spec);
		return false;
	}
	return true;
}

int main(int argc,char **argv) {
	uid_t uid = -1;
	gid_t gid = -1;

	if(argc < 3 || getopt_ishelp(argc,argv))
		usage(argv[0]);

	const char *spec = argv[1];
	if(!parseUserGroup(spec,&uid,&gid))
		exit(EXIT_FAILURE);

	for(int i = 2; i < argc; ++i) {
		if(chown(argv[i],uid,gid) < 0)
			printe("Unable to set owner/group of '%s' to %u/%u",argv[i],uid,gid);
	}
	return EXIT_SUCCESS;
}
