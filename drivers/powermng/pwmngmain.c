/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/debug.h>
#include <esc/dir.h>
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/ports.h>
#include <esc/proc.h>
#include <messages.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errors.h>

#define IOPORT_KB_DATA				0x60
#define IOPORT_KB_CTRL				0x64
#define KBC_CMD_RESET				0xFE

#define ARRAY_INC_SIZE				16
#define PROC_BUFFER_SIZE			512
#define WAIT_TIMEOUT				1000

static void killProcs(void);
static int pidCompare(const void *p1,const void *p2);
static void waitForProc(tPid pid);
static void getProcName(tPid pid,char *name);

static sMsg msg;

int main(void) {
	tDrvId id;
	tMsgId mid;
	bool run = true;

	if(requestIOPort(IOPORT_KB_DATA) < 0)
		error("Unable to request io-port %d",IOPORT_KB_DATA);
	if(requestIOPort(IOPORT_KB_CTRL) < 0)
		error("Unable to request io-port %d",IOPORT_KB_CTRL);

	id = regDriver("powermng",0);
	if(id < 0)
		error("Unable to register driver 'powermng'");

	/* wait for commands */
	while(run) {
		tFD fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[PWMNG] Unable to get work");
		else {
			switch(mid) {
				case MSG_POWER_REBOOT:
					killProcs();
					debugf("Using pulse-reset line of 8042 controller to reset...\n");
					run = false;
					/* wait until in-buffer empty */
					while((inByte(IOPORT_KB_CTRL) & 0x2) != 0);
					/* command 0xD1 to write the outputport */
					outByte(IOPORT_KB_CTRL,0xD1);
					/* wait again until in-buffer empty */
					while((inByte(IOPORT_KB_CTRL) & 0x2) != 0);
					/* now set the new output-port for reset */
					outByte(IOPORT_KB_DATA,0xFE);
					break;
				case MSG_POWER_SHUTDOWN:
					killProcs();
					debugf("System is stopped\n");
					debugf("You can turn off now\n");
					/* TODO we should use ACPI later here */
					break;
			}
			close(fd);
		}
	}

	/* clean up */
	releaseIOPort(IOPORT_KB_DATA);
	releaseIOPort(IOPORT_KB_CTRL);
	unregDriver(id);
	return EXIT_SUCCESS;
}

static void killProcs(void) {
	char name[MAX_PROC_NAME_LEN + 1];
	sDirEntry e;
	tFD fd;
	tPid pid,own = getpid();
	u32 i,pidSize = ARRAY_INC_SIZE;
	u32 pidPos = 0;

	tPid *pids = (tPid*)malloc(sizeof(tPid) * pidSize);
	if(pids == NULL)
		error("Unable to alloc mem for pids");

	fd = opendir("/system/processes");
	if(fd < 0) {
		free(pids);
		error("Unable to open '/system/processes'");
	}

	while(readdir(&e,fd)) {
		if(strcmp(e.name,".") == 0 || strcmp(e.name,"..") == 0)
			continue;
		pid = atoi(e.name);
		if(pid == 0 || pid == own)
			continue;
		if(pidPos >= pidSize) {
			pidSize += ARRAY_INC_SIZE;
			pids = (tPid*)realloc(pids,sizeof(tPid) * pidSize);
			if(pids == NULL)
				error("Unable to alloc mem for pids");
		}
		pids[pidPos++] = pid;
	}
	closedir(fd);

	qsort(pids,pidPos,sizeof(tPid),pidCompare);
	for(i = 0; i < pidPos; i++) {
		getProcName(pids[i],name);
		debugf("Terminating process %d (%s)",pids[i],name);
		if(sendSignalTo(pids[i],SIG_TERM,0) < 0)
			printe("[PWMNG] Unable to send the term-signal to %d",pids[i]);
		waitForProc(pids[i]);
	}

	free(pids);
}

static int pidCompare(const void *p1,const void *p2) {
	/* descending */
	return *(tPid*)p2 - *(tPid*)p1;
}

static void waitForProc(tPid pid) {
	tFD fd;
	u32 time = 0;
	char path[SSTRLEN("/system/processes/") + 12];
	snprintf(path,sizeof(path),"/system/processes/%d",pid);
	while(1) {
		fd = open(path,IO_READ);
		if(fd < 0)
			break;
		close(fd);
		if(time >= WAIT_TIMEOUT) {
			debugf("\nProcess does still exist after %d ms; killing it",time,pid);
			if(sendSignalTo(pid,SIG_KILL,0) < 0)
				printe("[PWMNG] Unable to send the kill-signal to %d",pid);
			break;
		}
		debugf(".");
		sleep(20);
		time += 20;
	}
	debugf("\n");
}

static void getProcName(tPid pid,char *name) {
	tFD fd;
	char buffer[PROC_BUFFER_SIZE];
	char path[SSTRLEN("/system/processes//info") + 12];
	snprintf(path,sizeof(path),"/system/processes/%d/info",pid);
	fd = open(path,IO_READ);
	/* maybe the process has just been terminated */
	if(fd < 0)
		return;
	if(read(fd,buffer,ARRAY_SIZE(buffer) - 1) < 0) {
		close(fd);
		return;
	}
	buffer[ARRAY_SIZE(buffer) - 1] = '\0';
	sscanf(
		buffer,
		"%*s%*hu" "%*s%*hu" "%*s%s",
		name
	);
	close(fd);
}
