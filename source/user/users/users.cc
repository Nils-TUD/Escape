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

#include <esc/proto/vterm.h>
#include <sys/cmdargs.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <sys/proc.h>
#include <usergroup/usergroup.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void listUsers(void);
static void addUser(const char *name,const char *home);
static void changeName(const char *old,const char *n);
static void changeHome(const char *name,const char *home);
static void changePw(const char *name);
static void deleteUser(const char *name);
static void readPassword(char *pw,size_t size);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <cmd>\n",name);
	fprintf(stderr,"    -l               : list all users\n");
	fprintf(stderr,"    -a <name> <home> : add new user <name>\n");
	fprintf(stderr,"    -n <old> <new>   : change name of user <old> to <new>\n");
	fprintf(stderr,"    -h <name> <home> : change home-dir of user <name>\n");
	fprintf(stderr,"    -p <name>        : change password of user <name>\n");
	fprintf(stderr,"    -d <name>        : delete user <name>\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	if(argc < 2 || argv[1][0] == '\0' || (argc == 2 && isHelpCmd(argc,argv)))
		usage(argv[0]);

	switch(argv[1][1]) {
		case 'l':
			if(argc != 2)
				usage(argv[0]);
			listUsers();
			break;
		case 'a':
		case 'n':
		case 'h':
			if(argc != 4)
				usage(argv[0]);
			if(argv[1][1] == 'a')
				addUser(argv[2],argv[3]);
			else if(argv[1][1] == 'n')
				changeName(argv[2],argv[3]);
			else
				changeHome(argv[2],argv[3]);
			break;
		case 'p':
		case 'd':
			if(argc != 3)
				usage(argv[0]);
			if(argv[1][1] == 'p')
				changePw(argv[2]);
			else
				deleteUser(argv[2]);
			break;
		default:
			usage(argv[0]);
			break;
	}
	return EXIT_SUCCESS;
}

static void listUsers(void) {
	sNamedItem *userList = usergroup_parse(USERS_PATH,NULL);
	if(!userList)
		error("Unable to load users");

	printf("%-20s %-4s %-4s %s\n","Name","UID","GID","Home");
	sNamedItem *u = userList;
	while(u != NULL) {
		char home[MAX_PATH_LEN];
		usergroup_getHome(u->name,home,sizeof(home));
		gid_t gid = usergroup_getGid(u->name);
		printf("%-20s %-4d %-4d %s\n",u->name,u->id,gid,home);
		u = u->next;
	}
}

static void addUser(const char *name,const char *home) {
	char path[MAX_PATH_LEN];

	sNamedItem *userList = usergroup_parse(USERS_PATH,NULL);
	if(!userList)
		error("Unable to load users");
	sNamedItem *groupList = usergroup_parse(GROUPS_PATH,NULL);
	if(!groupList)
		error("Unable to load groups");

	/* create user and group directories first. with this, we check existence at the same time */
	snprintf(path,sizeof(path),"%s/%s",USERS_PATH,name);
	if(mkdir(path,0755) < 0)
		error("Unable to create user directory '%s'",path);

	/* create group file first, so that we can use deleteUser to undo everything */
	int gid = usergroup_getFreeId(groupList);
	if(usergroup_storeIntAttr(USERS_PATH,name,"gid",gid,0644) < 0) {
		deleteUser(name);
		error("Unable to write gid");
	}

	snprintf(path,sizeof(path),"%s/%s",GROUPS_PATH,name);
	if(mkdir(path,0755) < 0) {
		deleteUser(name);
		error("Unable to create group directory '%s'",path);
	}

	int uid = usergroup_getFreeId(userList);
	if(usergroup_storeIntAttr(USERS_PATH,name,"id",uid,0644) < 0) {
		deleteUser(name);
		error("Unable to write uid");
	}

	if(usergroup_storeIntAttr(GROUPS_PATH,name,"id",gid,0644) < 0) {
		deleteUser(name);
		error("Unable to write id to group directory");
	}

	if(usergroup_storeStrAttr(USERS_PATH,name,"home",home,0644) < 0) {
		deleteUser(name);
		error("Unable to write home directory");
	}

	/* initially, the user is just in his own group */
	char grpStr[12];
	snprintf(grpStr,sizeof(grpStr),"%d",gid);
	if(usergroup_storeStrAttr(USERS_PATH,name,"groups",grpStr,0644) < 0) {
		deleteUser(name);
		error("Unable to write groups");
	}

	/* create password */
	char pw[MAX_PW_LEN];
	readPassword(pw,sizeof(pw));

	if(usergroup_storeStrAttr(USERS_PATH,name,"passwd",pw,0600) < 0) {
		deleteUser(name);
		error("Unable to write password");
	}

	snprintf(path,sizeof(path),"%s/%s/passwd",USERS_PATH,name);
	if(chown(path,uid,gid) < 0) {
		deleteUser(name);
		error("Unable to chown password file to user");
	}
}

static void changeName(const char *old,const char *n) {
	char oldPath[MAX_PATH_LEN];
	char newPath[MAX_PATH_LEN];
	snprintf(oldPath,sizeof(oldPath),"%s/%s",USERS_PATH,old);
	snprintf(newPath,sizeof(newPath),"%s/%s",USERS_PATH,n);
	if(rename(oldPath,newPath) < 0)
		error("Renaming user from '%s' to '%s' failed",old,n);
}

static void changeHome(const char *name,const char *home) {
	if(usergroup_storeStrAttr(USERS_PATH,name,"home",home,0644) < 0)
		error("Unable to write home directory");
}

static void changePw(const char *name) {
	char pw[MAX_PW_LEN];
	readPassword(pw,sizeof(pw));

	if(usergroup_storeStrAttr(USERS_PATH,name,"passwd",pw,0600) < 0)
		error("Unable to write password");
}

static void deleteUser(const char *name) {
	char path[MAX_PATH_LEN];

	/* first save the group id */
	int gid = usergroup_getGid(name);

	/* remove complete user directory */
	snprintf(path,sizeof(path),"rm -r %s/%s",USERS_PATH,name);
	system(path);

	/* if already existing and no longer in use, remove associated group directory */
	if(gid >= 0 && !usergroup_groupInUse(gid)) {
		const char *gname = usergroup_idToName(GROUPS_PATH,gid);
		if(gname) {
			snprintf(path,sizeof(path),"rm -r %s/%s",GROUPS_PATH,gname);
			system(path);
		}
	}
}

static void readPassword(char *pw,size_t size) {
	esc::VTerm vterm(STDOUT_FILENO);
	char secondPw[MAX_PW_LEN + 1];
	/* read in password */
	vterm.setFlag(esc::VTerm::FL_ECHO,false);
	printf("Password: ");
	fgetl(pw,size,stdin);
	putchar('\n');
	printf("Repeat: ");
	fgetl(secondPw,sizeof(secondPw),stdin);
	putchar('\n');
	vterm.setFlag(esc::VTerm::FL_ECHO,true);
	if(strcmp(pw,secondPw) != 0)
		error("Passwords do not match");
}
