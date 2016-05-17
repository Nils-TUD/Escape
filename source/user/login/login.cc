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
#include <esc/stream/std.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <usergroup/usergroup.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define SKIP_LOGIN			0
#define SHELL_PATH			"/bin/shell"
#define MAX_VTERM_NAME_LEN	10

using namespace esc;

static int getUser(const char *user,const char *pw);

int main(void) {
	char *termPath = getenv("TERM");
	char *termName = termPath + SSTRLEN("/dev/");

	/* we want to overwrite them */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	int fd;
	/* note: we do always pass O_MSGS to open because the user might want to request the console
	 * size or use isatty() or something. */
	if((fd = open(termPath,O_RDONLY | O_MSGS)) != STDIN_FILENO)
		exitmsg("Unable to open '" << termPath << "' for STDIN: Got fd " << fd);

	/* open stdout */
	if((fd = open(termPath,O_WRONLY | O_MSGS)) != STDOUT_FILENO)
		exitmsg("Unable to open '" << termPath << "' for STDOUT: Got fd " << fd);

	/* dup stdout to stderr */
	if((fd = dup(fd)) != STDERR_FILENO)
		exitmsg("Unable to duplicate STDOUT to STDERR: Got fd " << fd);

	/* refresh the istty property for stdin since the attempt during constructor calls failed */
	fisatty(stdin);

	sout << "\n\n";
	sout << "\033[co;9]Welcome to Escape v" << ESCAPE_VERSION << ", " << termName << "!\033[co]\n\n";
	sout << "Please login to get a shell.\n";
	sout << "Hint: use hrniels/test, jon/doe or root/root ;)\n\n";

	int uid;
	char un[MAX_NAME_LEN + 1];
	char pw[MAX_PW_LEN + 1];
	esc::VTerm vterm(STDOUT_FILENO);
	while(1) {
#if SKIP_LOGIN
		strcpy(un,"root");
		strcpy(pw,"root");
#else
		sout << "Username: ";
		sin.getline(un,sizeof(un));
		/* if an error occurred, e.g. the user pressed ^D, ensure that we unset the error */
		sin.clear();
		vterm.setFlag(esc::VTerm::FL_ECHO,false);

		sout << "Password: ";
		sin.getline(pw,sizeof(pw));
		sin.clear();
		vterm.setFlag(esc::VTerm::FL_ECHO,true);
		sout << '\n';
#endif

		uid = getUser(un,pw);
		if(uid >= 0)
			break;

		sout << "Sorry, invalid username or password. Try again!" << endl;
		sleep(1);
	}
	fflush(stdout);

	/* set user- and group-id */
	int gid = usergroup_getGid(un);
	if(gid < 0)
		error("Unable to get users gid");
	if(setgid(gid) < 0)
		exitmsg("Unable to set gid");
	if(setuid(uid) < 0)
		exitmsg("Unable to set uid");

	/* determine groups and set them */
	size_t groupCount;
	gid_t *groups = usergroup_collectGroupsFor(un,1,&groupCount);
	if(!groups)
		exitmsg("Unable to collect group-ids");
	int vtgid = usergroup_nameToId(GROUPS_PATH,termName);
	/* add the process to the corresponding ui-group */
	if(vtgid >= 0)
		groups[groupCount++] = vtgid;
	if(setgroups(groupCount,groups) < 0)
		exitmsg("Unable to set groups");

	/* use a per-user mountspace */
	char mspath[MAX_PATH_LEN];
	snprintf(mspath,sizeof(mspath),"/sys/ms/%s",un);
	int ms = open(mspath,O_RDONLY);
	if(ms < 0) {
		ms = open("/sys/proc/self/ms",O_RDONLY);
		if(ms < 0)
			exitmsg("Unable to open /sys/proc/self/ms for reading");
		if(clonems(ms,un) < 0)
			exitmsg("Unable to clone mountspace");
	}
	else {
		if(joinms(ms) < 0)
			exitmsg("Unable to join mountspace '" << mspath << "'");
	}
	close(ms);

	/* cd to home-dir */
	char homedir[MAX_PATH_LEN];
	if(usergroup_getHome(un,homedir,sizeof(homedir)) != 0)
		error("Unable to get users home directory");
	if(isdir(homedir))
		setenv("CWD",homedir);
	else
		setenv("CWD","/");
	setenv("HOME",getenv("CWD"));
	setenv("USER",un);

	/* exchange with shell */
	const char *shargs[] = {SHELL_PATH,NULL};
	execv(SHELL_PATH,shargs);

	/* not reached */
	return EXIT_SUCCESS;
}

static int getUser(const char *name,const char *pw) {
	int res = usergroup_nameToId(USERS_PATH,name);
	if(res < 0)
		return res;

	const char *upw = usergroup_getPW(name);
	if(upw && strcmp(upw,pw) == 0)
		return res;
	return -EINVAL;
}
