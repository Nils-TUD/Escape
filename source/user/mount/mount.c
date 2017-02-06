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
#include <sys/io.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/thread.h>
#include <sys/wait.h>
#include <dirent.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool run = true;

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [--ms <ms>] <device> <path> <fs>\n",name);
	fprintf(stderr,"    Creates a child process that executes <fs>. <fs> receives\n");
	fprintf(stderr,"    the fs-device to create and the device to work with (<device>)\n");
	fprintf(stderr,"    as command line arguments. Afterwards, mount opens the\n");
	fprintf(stderr,"    created fs-device and mounts it at <path>.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"    Example: %s /dev/hda1 /mnt /sbin/ext2.\n",name);
	fprintf(stderr,"\n");
	fprintf(stderr,"    By default, the current mountspace (/sys/proc/self/ms) will\n");
	fprintf(stderr,"    be used. This can be overwritten by specifying --ms <ms>.\n");
	exit(EXIT_FAILURE);
}

static void sigchild(A_UNUSED int sig) {
	run = false;
}

static void cleanup(int pid) {
	if(pid != -1) {
		kill(pid,SIGTERM);
		waitchild(NULL,pid);
	}
}

int main(int argc,const char *argv[]) {
	char fsname[MAX_PATH_LEN];
	char fsdev[MAX_PATH_LEN];
	char devpath[MAX_PATH_LEN];
	char *mspath = (char*)"/sys/proc/self/ms";
	char *path = NULL;
	char *dev = NULL;
	char *fs = NULL;

	int res = ca_parse(argc,argv,CA_NO_FREE,"ms=s =s* =s* =s*",&mspath,&dev,&path,&fs);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	if(signal(SIGCHLD,sigchild) == SIG_ERR)
		error("Unable to announce signal handler");

	/* build fs-device */
	strnzcpy(devpath,dev,sizeof(devpath));
	if(dev[0] == '/')
		dev++;
	for(size_t i = 0; dev[i]; ++i) {
		if(dev[i] == '/')
			dev[i] = '-';
	}

	strnzcpy(fsname,fs,sizeof(fsname));
	/* use dev-number and inode-number if it's a file */
	struct stat info;
	if(stat(devpath,&info) == 0)
		snprintf(fsdev,sizeof(fsdev),"/dev/%s-%s-%d-%d",basename(fsname),dev,info.st_dev,info.st_ino);
	else
		snprintf(fsdev,sizeof(fsdev),"/dev/%s-%s",basename(fsname),dev);

	/* create child to run the filesystem */
	int fd, pid = fork();
	if(pid < 0)
		error("fork failed");
	if(pid == 0) {
		const char *args[] = {fs,fsdev,devpath,NULL};
		execvp(fs,args);
		error("exec failed");
	}
	else {
		/* wait until fs-device is present */
		while(run && (fd = open(fsdev,O_MSGS)) == -ENOENT)
			usleep(5 * 1000);
		if(!run)
			errno = -ENOENT;
	}

	/* now mount it */
	int ms = open(mspath,O_WRITE);
	if(ms < 0) {
		printe("Unable to open '%s' for writing",mspath);
		cleanup(pid);
		return EXIT_FAILURE;
	}
	if(mount(ms,fd,path) < 0) {
		printe("Unable to mount '%s' @ '%s' with fs %s in mountspace '%s'",dev,path,fs,mspath);
		cleanup(pid);
		return EXIT_FAILURE;
	}

	close(ms);
	close(fd);
	return EXIT_SUCCESS;
}
