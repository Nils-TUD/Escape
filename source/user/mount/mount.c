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
#include <dirent.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool run = true;

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [--ms <ms>] <device> <path> <fs>\n",name);
	fprintf(stderr,"    For example, %s /dev/hda1 /mnt ext2, where ext2 is a program\n",name);
	fprintf(stderr,"    in PATH that takes the device as argument 2 and creates\n");
	fprintf(stderr,"    /dev/ext2-hda1 (in this case).\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"    By default, the current mountspace (/sys/proc/self/ms) will\n");
	fprintf(stderr,"    be used. This can be overwritten by specifying --ms <ms>.\n");
	exit(EXIT_FAILURE);
}

static void sigchild(A_UNUSED int sig) {
	run = false;
}

int main(int argc,const char *argv[]) {
	char devpath[MAX_PATH_LEN];
	char fsname[MAX_PATH_LEN];
	char fsdev[MAX_PATH_LEN];
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
	cleanpath(devpath,sizeof(devpath),dev);
	char *devname = devpath + 1;
	if(strncmp(devname,"dev/",4) == 0)
		devname += 4;
	for(size_t i = 0; devname[i]; ++i) {
		if(devname[i] == '/')
			devname[i] = '-';
	}

	strnzcpy(fsname,fs,sizeof(fsname));
	snprintf(fsdev,sizeof(fsdev),"/dev/%s-%s",basename(fsname),devname);

	/* is it already started? */
	int fd = open(fsdev,O_MSGS);
	if(fd == -ENOENT) {
		/* ok, do so now */
		int pid = fork();
		if(pid < 0)
			error("fork failed");
		if(pid == 0) {
			const char *args[] = {fs,fsdev,dev,NULL};
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
	}
	if(fd < 0)
		error("Unable to open '%s'",fsdev);

	/* now mount it */
	int ms = open(mspath,O_WRITE);
	if(ms < 0)
		error("Unable to open '%s' for writing",mspath);
	if(mount(ms,fd,path) < 0)
		error("Unable to mount '%s' @ '%s' with fs %s in mountspace '%s'",dev,path,fs,mspath);

	close(ms);
	close(fd);
	return EXIT_SUCCESS;
}
