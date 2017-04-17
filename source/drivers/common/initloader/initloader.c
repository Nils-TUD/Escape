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
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/syscalls.h>
#include <sys/sysctrace.h>
#include <sys/thread.h>
#include <usergroup/usergroup.h>
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ARG_COUNT		32
#define MAX_ARG_LEN			256

static char **parseArgs(const char *line,int *argc) {
	static char argvals[MAX_ARG_COUNT][MAX_ARG_LEN];
	static char *args[MAX_ARG_COUNT];
	size_t i = 0,j = 0;
	args[0] = argvals[0];
	while(*line) {
		if(*line == ' ') {
			if(args[j][0]) {
				if(j + 2 >= MAX_ARG_COUNT)
					break;
				args[j][i] = '\0';
				j++;
				i = 0;
				args[j] = argvals[j];
			}
		}
		else if(i < MAX_ARG_LEN)
			args[j][i++] = *line;
		line++;
	}
	*argc = j + 1;
	args[j][i] = '\0';
	args[j + 1] = NULL;
	return args;
}

extern char __progname[];

/* we have to hardcode the uids and gids for the boot modules here, because we have no root fs yet */
static const struct {
	const char *match;
	uid_t uid;
	size_t gcount;
	gid_t gids[4];
} bootModUsers[] = {
	{"pci",		USER_BUS,		2, {GROUP_BUS,GROUP_DRIVER,0,0}},
	{"ata",		USER_STORAGE,	3, {GROUP_STORAGE,GROUP_DRIVER,GROUP_BUS,0}},
	{"disk",	USER_STORAGE,	2, {GROUP_STORAGE,GROUP_DRIVER,0,0}},
	{"ramdisk",	USER_STORAGE,	2, {GROUP_STORAGE,GROUP_DRIVER,0,0}},
	{"iso9660",	USER_FS,		3, {GROUP_FS,GROUP_STORAGE,GROUP_DRIVER,0}},
	{"ext2",	USER_FS,		3, {GROUP_FS,GROUP_STORAGE,GROUP_DRIVER,0}},
};

int main(void) {
	/* give ourself a name */
	strcpy(__progname,"initloader");

	if(getpid() > 1)
		error("It's not good to start init twice ;)");

	/* perform some additional initialization in the kernel */
	/* on MMIX, this needs multiple steps */
	int res = 0;
	do
		res = syscall0(SYSCALL_INIT);
	while(res == 0);

	/* set sync file */
	char tmp[12];
	uint32_t cnt = 0;
	int straceFd = open("/sys/strace",O_CREAT | O_TRUNC | O_RDWR,0666);
	if(straceFd != STRACE_FILENO)
		error("Got the wrong fd for /sys/strace");
	sassert(write(straceFd,&cnt,sizeof(cnt)) == sizeof(cnt));
	itoa(tmp,sizeof(tmp),straceFd);
	setenv("SYSCTRC_SYNCFD",tmp);

	/* enable syscall tracing */
	if(sysconf(CONF_LOG_SYSCALLS)) {
		int max = sysconf(CONF_MAX_FDS);
		/* set file descriptor to an invalid value to log the trace */
		itoa(tmp,sizeof(tmp),max);
		setenv("SYSCTRC_FD",tmp);
		/* reinit syscall tracing */
		traceFd = -2;
	}

	/* open arguments file */
	char line[256];
	char filepath[MAX_PATH_LEN];
	FILE *args = fopen("/sys/boot/arguments","r");
	if(!args)
		error("Unable to open /sys/boot/arguments for reading");

	/* load boot modules */
	for(int i = 0; !feof(args); ++i) {
		fgetl(line,sizeof(line),args);
		/* skip ourself */
		if(*line == '\0' || i == 0)
			continue;

		int argc;
		char **argv = parseArgs(line,&argc);
		/* ignore modules with no wait-argument (like ramdisks) */
		if(argc < 2)
			continue;

		/* all boot-modules are in /sys/boot */
		snprintf(filepath,sizeof(filepath),"/sys/boot/%s",basename(argv[0]));
		argv[0] = filepath;

		int pid;
		if((pid = fork()) == 0) {
			/* set uid, gid and groups */
			for(size_t x = 0; x < ARRAY_SIZE(bootModUsers); ++x) {
				if(strstr(argv[0],bootModUsers[x].match)) {
					if(setgroups(bootModUsers[x].gcount,bootModUsers[x].gids) < 0)
						error("Unable to set groups");
					if(setgid(bootModUsers[x].gids[0]) < 0)
						error("Unable to set group %d",bootModUsers[x].gids[0]);
					if(setuid(bootModUsers[x].uid) < 0)
						error("Unable to set user %d",bootModUsers[x].uid);
					break;
				}
			}

			execv(argv[0],(const char**)argv);
			error("exec failed");
		}
		else if(pid < 0)
			error("fork failed");
		else {
			/* wait for the device to be available */
			struct stat info;
			int res;
			while((res = stat(argv[1],&info)) == -ENOENT)
				usleep(20 * 1000);
			if(res < 0)
				error("stat for %s failed",argv[1]);
		}
	}
	fclose(args);

	/* mount root device */
	if(sysconfstr(CONF_ROOT_DEVICE,line,sizeof(line)) < 0)
		error("Unable to get root device");
	int fd = open(line,O_RDWR | O_EXEC);
	if(fd < 0)
		error("Unable to open '%s'",line);
	/* no fch{own,mod} here, because we want to change the device, not the channel */
	if(chown(line,ROOT_UID,GROUP_FS) < 0)
		printe("Warning: unable to set owner of %s",line);
	if(chmod(line,0770) < 0)
		printe("Warning: unable to set permissions of %s",line);

	/* clone mountspace */
	if(clonems("root") < 0)
		error("Unable to clone mountspace");

	/* mount root filesystem */
	int ms = open("/sys/pid/self/ms",O_RDWR);
	if(ms < 0)
		error("Unable to open '/sys/pid/self/ms'");
	if(fchmod(ms,0755) < 0)
		error("Unable to chmod '/sys/pid/self/ms'");
	if(mount(ms,fd,"/") < 0)
		error("Unable to mount '%s' at '/'",line);
	close(ms);
	close(fd);

	/* exec init */
	const char *argv[] = {"/sbin/init",NULL};
	execv(argv[0],argv);
	return 0;
}
