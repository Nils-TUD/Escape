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

#include <sys/cmdargs.h>
#include <sys/proc.h>
#include <usergroup/usergroup.h>
#include <stdio.h>
#include <stdlib.h>

static void printUser(int uid,bool name);
static void printGroup(int gid,bool name);
static void printAll(int uid,int gid,gid_t *gids,size_t count);
static void printAllGroups(gid_t *gids,size_t count,bool name);

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-g] [-G] [-u] [-n] [<user>]\n",name);
	fprintf(stderr,"  -g: print only effective group id/name\n");
	fprintf(stderr,"  -u: print only effective user id/name\n");
	fprintf(stderr,"  -G: print all group ids/names\n");
	fprintf(stderr,"  -n: print names instead of ids\n");
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	int g = 0,G = 0,u = 0,n = 0;

	/* parse args */
	int res = ca_parse(argc,argv,CA_MAX1_FREE,"g G u n",&g,&G,&u,&n);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	int uid,gid;
	gid_t *gids;
	size_t gcount;
	const char *user = ca_getFree()[0];
	if(user) {
		uid = usergroup_nameToId(USERS_PATH,user);
		gid = usergroup_getGid(user);
		gids = usergroup_collectGroupsFor(user,0,&gcount);
		if(!gids)
			error("Unable to get group ids for user '%s'",user);
	}
	else {
		uid = geteuid();
		gid = getegid();
		gcount = getgroups(0,NULL);
		gids = (gid_t*)malloc(sizeof(gid_t) * gcount);
		if(getgroups(gcount,gids) < 0)
			error("Unable to get group ids of current process");
	}

	if(!G && !g && !u) {
		if(n)
			error("Invalid arguments: cannot use -n without -g or -u");
		printAll(uid,gid,gids,gcount);
	}
	else if(g)
		printGroup(gid,n);
	else if(G)
		printAllGroups(gids,gcount,n);
	else
		printUser(uid,n);
	printf("\n");

	free(gids);
	return 0;
}

static void printUser(int uid,bool name) {
	if(name)
		printf("%s",usergroup_idToName(USERS_PATH,uid));
	else
		printf("%d",uid);
}

static void printGroup(int gid,bool name) {
	if(name)
		printf("%s",usergroup_idToName(GROUPS_PATH,gid));
	else
		printf("%d",gid);
}

static void printAll(int uid,int gid,gid_t *gids,size_t count) {
	printf("uid=");
	printUser(uid,false);
	printf("(");
	printUser(uid,true);
	printf(") ");

	printf("gid=");
	printGroup(gid,false);
	printf("(");
	printGroup(gid,true);
	printf(") ");

	printf("groups=");
	for(size_t i = 0; i < count; ++i) {
		printGroup(gids[i],false);
		printf("(");
		printGroup(gids[i],true);
		printf(")");
		if(i + 1 < count)
			printf(",");
	}
}

static void printAllGroups(gid_t *gids,size_t count,bool name) {
	for(size_t i = 0; i < count; ++i) {
		printGroup(gids[i],name);
		if(i + 1 < count)
			printf(" ");
	}
}
