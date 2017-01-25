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
#include <sys/common.h>
#include <sys/messages.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <usergroup/usergroup.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void listAllGroups(void);
static void listCurGroups(void);
static void listGroupsOf(const char *user);
static void listGroups(gid_t *gids,size_t count);
static void addGroup(const char *name);
static void changeName(const char *old,const char *n);
static void join(const char *user,const char *group);
static void leave(const char *user,const char *group);
static void writeGroupIds(const char *user,gid_t *gids,size_t count);
static void deleteGroup(const char *name);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <cmd>\n",name);
	fprintf(stderr,"    -l               : list all groups\n");
	fprintf(stderr,"    -g               : list of groups of the current process\n");
	fprintf(stderr,"    -u <name>        : list of groups of the given user\n");
	fprintf(stderr,"    -a <name>        : add new group <name>\n");
	fprintf(stderr,"    -n <old> <new>   : change name of group <old> to <new>\n");
	fprintf(stderr,"    -j <user> <name> : user <user> joins group <name>\n");
	fprintf(stderr,"    -r <user> <name> : user <user> leaves group <name>\n");
	fprintf(stderr,"    -d <name>        : delete group <name>\n");
	exit(EXIT_FAILURE);
}

static sNamedItem *groupList;
static sNamedItem *userList;

int main(int argc,char *argv[]) {
	size_t count;
	if(argc < 2 || argv[1][0] == '\0' || isHelpCmd(argc,argv))
		usage(argv[0]);

	groupList = usergroup_parse(GROUPS_PATH,&count);
	if(!groupList)
		error("Unable to load groups");
	userList = usergroup_parse(USERS_PATH,&count);
	if(!userList)
		error("Unable to load users");

	switch(argv[1][1]) {
		case 'l':
		case 'g':
			if(argc != 2)
				usage(argv[0]);
			if(argv[1][1] == 'l')
				listAllGroups();
			else
				listCurGroups();
			break;
		case 'u':
			if(argc != 3)
				usage(argv[0]);
			listGroupsOf(argv[2]);
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

static void listAllGroups(void) {
	sNamedItem *g = groupList;
	printf("%-20s %-4s %s\n","Name","GID","Members");
	while(g != NULL) {
		printf("%-20s %-4d ",g->name,g->id);

		sNamedItem *u = userList;
		while(u != NULL) {
			size_t count;
			gid_t *gids = usergroup_collectGroupsFor(u->name,0,&count);
			if(gids) {
				for(size_t i = 0; i < count; ++i) {
					if(gids[i] == g->id) {
						printf("%s ",u->name);
						break;
					}
				}
				free(gids);
			}
			u = u->next;
		}
		printf("\n");

		g = g->next;
	}
}

static void listGroupsOf(const char *user) {
	size_t count;
	gid_t *gids = usergroup_collectGroupsFor(user,0,&count);
	listGroups(gids,count);
	free(gids);
}

static void listCurGroups(void) {
	int count = getgroups(0,NULL);
	if(count < 0)
		error("Unable to get groups of current process");

	gid_t *gids = (gid_t*)malloc(count * sizeof(gid_t));
	if(!gids)
		error("malloc");
	count = getgroups(count,gids);
	if(count < 0)
		error("Unable to get groups of current process");

	listGroups(gids,count);
	free(gids);
}

static void listGroups(gid_t *gids,size_t count) {
	for(size_t i = 0; i < count; i++) {
		const char *name = usergroup_idToName(GROUPS_PATH,gids[i]);
		if(name)
			printf("%s ",name);
	}
	printf("\n");
}

static void addGroup(const char *name) {
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"%s/%s",GROUPS_PATH,name);
	if(mkdir(path,0755) < 0)
		error("Unable to create group directory '%s'",path);

	int gid = usergroup_getFreeId(groupList);
	if(usergroup_storeIntAttr(GROUPS_PATH,name,"id",gid,0644) < 0) {
		if(rmdir(path) < 0)
			printe("Unable to remove the created group directory");
		error("Unable to write id to group directory");
	}
}

static void changeName(const char *old,const char *n) {
	char oldPath[MAX_PATH_LEN];
	char newPath[MAX_PATH_LEN];
	snprintf(oldPath,sizeof(oldPath),"%s/%s",GROUPS_PATH,old);
	snprintf(newPath,sizeof(newPath),"%s/%s",GROUPS_PATH,n);
	if(rename(oldPath,newPath) < 0)
		error("Renaming group from '%s' to '%s' failed",old,n);
}

static void join(const char *user,const char *group) {
	int gid = usergroup_nameToId(GROUPS_PATH,group);
	if(gid < 0)
		error("Unable to get id of group '%s'",group);

	size_t count;
	gid_t *gids = usergroup_collectGroupsFor(user,1,&count);
	gids[count++] = gid;

	writeGroupIds(user,gids,count);
	free(gids);
}

static void leave(const char *user,const char *group) {
	int gid = usergroup_nameToId(GROUPS_PATH,group);
	if(gid < 0)
		error("Unable to get id of group '%s'",group);

	/* remove group id from list */
	bool found = false;
	size_t count;
	gid_t *gids = usergroup_collectGroupsFor(user,0,&count);
	for(size_t i = 0; i < count; ++i) {
		if(gids[i] == gid) {
			count--;
			found = true;
		}
		if(found && i + 1 < count)
			gids[i] = gids[i + 1];
	}

	writeGroupIds(user,gids,count);
	free(gids);
}

static void writeGroupIds(const char *user,gid_t *gids,size_t count) {
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"%s/%s/groups",USERS_PATH,user);
	FILE *f = fopen(path,"w");
	if(!f)
		error("Unable to open '%s' for writing",path);
	for(size_t i = 0; i < count; ++i) {
		fprintf(f,"%d",gids[i]);
		if(i + 1 < count)
			fprintf(f,",");
	}
	fclose(f);
}

static void deleteGroup(const char *name) {
	char path[MAX_PATH_LEN];

	int gid = usergroup_nameToId(GROUPS_PATH,name);
	if(gid < 0)
		error("Unable to get id of group '%s'",name);

	/* let all users leave the group first */
	sNamedItem *u = userList;
	while(u != NULL) {
		size_t count;
		gid_t *gids = usergroup_collectGroupsFor(u->name,0,&count);
		if(gids) {
			for(size_t i = 0; i < count; ++i) {
				if(gids[i] == gid) {
					leave(u->name,name);
					break;
				}
			}
			free(gids);
		}
		u = u->next;
	}

	/* remove complete group directory */
	snprintf(path,sizeof(path),"rm -r %s/%s",GROUPS_PATH,name);
	system(path);
}
