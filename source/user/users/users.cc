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
#include <usergroup/group.h>
#include <usergroup/passwd.h>
#include <usergroup/user.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void listUsers(void);
static void addUser(const char *name,const char *home);
static void changeName(const char *old,const char *n);
static void changeHome(const char *name,const char *home);
static void changePw(const char *name);
static void deleteUser(const char *name);
static void readPassword(sPasswd *p);
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

static sUser *userList;
static sPasswd *pwList;

int main(int argc,char *argv[]) {
	size_t usercount,pwcount;
	if(argc < 2 || argv[1][0] == '\0' || (argc == 2 && isHelpCmd(argc,argv)))
		usage(argv[0]);

	/* we need to be root to read/write /etc/users and so on */
	if(seteuid(ROOT_UID) < 0)
		error("Unable to set euid to ROOT_UID");

	userList = user_parseFromFile(USERS_PATH,&usercount);
	if(!userList)
		error("Unable to parse users-file '%s'",USERS_PATH);
	pwList = pw_parseFromFile(PASSWD_PATH,&pwcount);
	if(!pwList)
		error("Unable to parse users-file '%s'",PASSWD_PATH);

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
	sUser *u = userList;
	printf("%-20s %-4s %-4s %s\n","Name","UID","GID","Home");
	while(u != NULL) {
		printf("%-20s %-4d %-4d %s\n",u->name,u->uid,u->gid,u->home);
		u = u->next;
	}
}

static void addUser(const char *name,const char *home) {
	size_t count;
	sUser *u;
	sPasswd *p;
	sGroup *g,*groupList = group_parseFromFile(GROUPS_PATH,&count);
	if(!groupList)
		error("Unable to parse groups from '%s'",GROUPS_PATH);

	/* check existence */
	if(user_getByName(userList,name) != NULL)
		error("A user with name '%s' does already exist",name);
	if(group_getByName(groupList,name) != NULL)
		error("A group with name '%s' does already exist",name);

	/* create group for user */
	g = (sGroup*)malloc(sizeof(sGroup));
	if(!g)
		error("malloc");
	g->gid = group_getFreeGid(groupList);
	strnzcpy(g->name,name,sizeof(g->name));
	g->userCount = 1;
	g->users = (uid_t*)malloc(sizeof(uid_t) * 1);
	if(!g->users)
		error("malloc");
	group_append(groupList,g);

	/* create user */
	u = (sUser*)malloc(sizeof(sUser));
	if(!u)
		error("malloc");
	u->gid = g->gid;
	u->uid = user_getFreeUid(userList);
	strnzcpy(u->name,name,sizeof(u->name));
	strnzcpy(u->home,home,sizeof(u->home));
	g->users[0] = u->uid;
	user_append(userList,u);

	/* create password */
	p = (sPasswd*)malloc(sizeof(sPasswd));
	p->uid = u->uid;
	readPassword(p);
	pw_append(pwList,p);

	/* write to file */
	if(user_writeToFile(userList,USERS_PATH) < 0)
		error("Unable to write users back to file");
	if(group_writeToFile(groupList,GROUPS_PATH) < 0)
		error("Unable to write groups back to file");
	if(pw_writeToFile(pwList,PASSWD_PATH) < 0)
		error("Unable to write passwords back to file");
}

static void changeName(const char *old,const char *n) {
	uid_t uid = getuid();
	sUser *u = user_getByName(userList,old);
	if(!u)
		error("A user with name '%s' does not exist",old);
	if(uid != ROOT_UID && u->uid != uid)
		error("Only root can do that!");
	if(user_getByName(userList,n) != NULL)
		error("A user with name '%s' does already exist",n);

	strnzcpy(u->name,n,sizeof(u->name));

	if(user_writeToFile(userList,USERS_PATH) < 0)
		error("Unable to write users back to file");
}

static void changeHome(const char *name,const char *home) {
	uid_t uid = getuid();
	sUser *u = user_getByName(userList,name);
	if(!u)
		error("A user with name '%s' does not exist",name);
	if(uid != ROOT_UID && u->uid != uid)
		error("Only root can do that!");

	strnzcpy(u->home,home,sizeof(u->home));

	if(user_writeToFile(userList,USERS_PATH) < 0)
		error("Unable to write users back to file");
}

static void changePw(const char *name) {
	uid_t uid = getuid();
	sPasswd *p;
	const sUser *u = user_getByName(userList,name);
	if(!u)
		error("A user with name '%s' does not exist",name);
	if(uid != ROOT_UID && u->uid != uid)
		error("Only root can do that!");
	p = pw_getById(pwList,u->uid);
	if(!p)
		error("Unable to find password-entry for user '%s'",name);

	readPassword(p);

	if(pw_writeToFile(pwList,PASSWD_PATH) < 0)
		error("Unable to write passwords back to file");
}

static void deleteUser(const char *name) {
	size_t count;
	sGroup *g,*groupList;
	sPasswd *p;
	sUser *u = user_getByName(userList,name);
	if(!u)
		error("A user with name '%s' does not exist",name);
	if(getuid() != ROOT_UID)
		error("Only root can do that!");

	groupList = group_parseFromFile(GROUPS_PATH,&count);
	if(!groupList)
		error("Unable to parse groups from '%s'",GROUPS_PATH);
	g = group_getById(groupList,u->gid);
	if(!g)
		error("Group with id %d not found",u->gid);
	p = pw_getById(pwList,u->uid);
	if(!p)
		error("Unable to find password-entry for user '%s'",name);

	/* remove group if it contains exactly this user  */
	if(g->userCount == 1 && g->users[0] == u->uid)
		group_remove(groupList,g);
	/* remove user and uid from all groups */
	group_removeFromAll(groupList,u->uid);
	pw_remove(pwList,p);
	user_remove(userList,u);

	if(user_writeToFile(userList,USERS_PATH) < 0)
		error("Unable to write users back to file");
	if(group_writeToFile(groupList,GROUPS_PATH) < 0)
		error("Unable to write groups back to file");
	if(pw_writeToFile(pwList,PASSWD_PATH) < 0)
		error("Unable to write passwords back to file");
}

static void readPassword(sPasswd *p) {
	esc::VTerm vterm(STDOUT_FILENO);
	char secondPw[MAX_PW_LEN + 1];
	/* read in password */
	vterm.setFlag(esc::VTerm::FL_ECHO,false);
	printf("Password: ");
	fgetl(p->pw,sizeof(p->pw),stdin);
	putchar('\n');
	printf("Repeat: ");
	fgetl(secondPw,sizeof(secondPw),stdin);
	putchar('\n');
	vterm.setFlag(esc::VTerm::FL_ECHO,true);
	if(strcmp(p->pw,secondPw) != 0)
		error("Passwords do not match");
}
