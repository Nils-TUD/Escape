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
#include <usergroup/user.h>
#include <usergroup/group.h>
#include <esc/messages.h>
#include <esc/cmdargs.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void listGroups(void);
static void listCurGroups(void);
static void addGroup(const char *name);
static void changeName(const char *old,const char *new);
static void join(const char *user,const char *group);
static void leave(const char *user,const char *group);
static void deleteGroup(const char *name);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <cmd>\n",name);
	fprintf(stderr,"    -l               : list all groups\n");
	fprintf(stderr,"    -g               : list of groups of the current process\n");
	fprintf(stderr,"    -a <name>        : add new group <name>\n");
	fprintf(stderr,"    -n <old> <new>   : change name of group <old> to <new>\n");
	fprintf(stderr,"    -j <user> <name> : user <user> joins group <name>\n");
	fprintf(stderr,"    -r <user> <name> : user <user> leaves group <name>\n");
	fprintf(stderr,"    -d <name>        : delete group <name>\n");
	exit(EXIT_FAILURE);
}

static sGroup *groupList;
static sUser *userList;

int main(int argc,char *argv[]) {
	size_t count;
	if(argc < 2 || argv[1][0] == '\0' || isHelpCmd(argc,argv))
		usage(argv[0]);

	/* we need to be root to write /etc/groups */
	if(seteuid(ROOT_UID) < 0)
		error("Unable to set euid to ROOT_UID");

	groupList = group_parseFromFile(GROUPS_PATH,&count);
	if(!groupList)
		error("Unable to parse groups-file '%s'",GROUPS_PATH);
	userList = user_parseFromFile(USERS_PATH,&count);
	if(!userList)
		error("Unable to parse users-file '%s'",USERS_PATH);

	switch(argv[1][1]) {
		case 'l':
		case 'g':
			if(argc != 2)
				usage(argv[0]);
			if(argv[1][1] == 'l')
				listGroups();
			else
				listCurGroups();
			break;
		case 'a':
		case 'd':
			if(argc != 3)
				usage(argv[0]);
			if(argv[1][1] == 'a')
				addGroup(argv[2]);
			else
				deleteGroup(argv[2]);
			break;
		case 'n':
		case 'j':
		case 'r':
			if(argc != 4)
				usage(argv[0]);
			if(argv[1][1] == 'n')
				changeName(argv[2],argv[3]);
			else if(argv[1][1] == 'j')
				join(argv[2],argv[3]);
			else
				leave(argv[2],argv[3]);
			break;
		default:
			usage(argv[0]);
	}
	return EXIT_SUCCESS;
}

static void listGroups(void) {
	sGroup *g = groupList;
	printf("%-20s %-4s %s\n","Name","GID","Members");
	while(g != NULL) {
		size_t i;
		printf("%-20s %-4d ",g->name,g->gid);
		for(i = 0; i < g->userCount; i++) {
			sUser *u = user_getById(userList,g->users[i]);
			if(!u)
				error("User with id %d does not exist!?",g->users[i]);
			printf("%s ",u->name);
		}
		printf("\n");
		g = g->next;
	}
}

static void listCurGroups(void) {
	gid_t *groupIds;
	int i,count = getgroups(0,NULL);
	if(count < 0)
		error("Unable to get groups of current process");

	groupIds = (gid_t*)malloc(count * sizeof(gid_t));
	if(!groupIds)
		error("malloc");
	count = getgroups(count,groupIds);
	if(count < 0)
		error("Unable to get groups of current process");

	for(i = 0; i < count; i++) {
		sGroup *g = group_getById(groupList,groupIds[i]);
		printf("%s ",g->name);
	}
	printf("\n");
}

static void addGroup(const char *name) {
	uid_t uid = getuid();
	sGroup *g;
	if(group_getByName(groupList,name) != NULL)
		error("A group with name '%s' does already exist",name);
	if(uid != ROOT_UID)
		error("Only root can do that!");

	g = (sGroup*)malloc(sizeof(sGroup));
	if(!g)
		error("malloc");
	g->gid = group_getFreeGid(groupList);
	strnzcpy(g->name,name,sizeof(g->name));
	g->userCount = 0;
	g->users = NULL;
	group_append(groupList,g);

	if(group_writeToFile(groupList,GROUPS_PATH) < 0)
		error("Unable to write groups back to file");
}

static void changeName(const char *old,const char *new) {
	uid_t uid = getuid();
	sGroup *g = group_getByName(groupList,old);
	if(!g)
		error("A group with name '%s' does not exist",old);
	if(uid != ROOT_UID)
		error("Only root can do that!");

	strnzcpy(g->name,new,sizeof(g->name));

	if(group_writeToFile(groupList,GROUPS_PATH) < 0)
		error("Unable to write groups back to file");
}

static void join(const char *user,const char *group) {
	uid_t uid = getuid();
	sUser *u = user_getByName(userList,user);
	sGroup *g = group_getByName(groupList,group);
	if(!g || !u)
		error("Group or user does not exist");
	if(uid != ROOT_UID)
		error("Only root can do that!");

	g->userCount++;
	g->users = (uid_t*)realloc(g->users,g->userCount);
	g->users[g->userCount - 1] = u->uid;

	if(group_writeToFile(groupList,GROUPS_PATH) < 0)
		error("Unable to write groups back to file");
}

static void leave(const char *user,const char *group) {
	uid_t uid = getuid();
	sUser *u = user_getByName(userList,user);
	sGroup *g = group_getByName(groupList,group);
	if(!g || !u)
		error("Group or user does not exist");
	if(uid != ROOT_UID)
		error("Only root can do that!");

	group_removeFrom(g,u->uid);

	if(group_writeToFile(groupList,GROUPS_PATH) < 0)
		error("Unable to write groups back to file");
}

static void deleteGroup(const char *name) {
	uid_t uid = getuid();
	sGroup *g = group_getByName(groupList,name);
	if(!g)
		error("A group with name '%s' does not exist",name);
	if(uid != ROOT_UID)
		error("Only root can do that!");

	group_remove(groupList,g);

	if(group_writeToFile(groupList,GROUPS_PATH) < 0)
		error("Unable to write groups back to file");
}
