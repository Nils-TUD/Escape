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
#include <sys/task/proc.h>
#include <sys/task/smp.h>
#include <sys/vfs/vfs.h>
#include <sys/util.h>
#include <esc/messages.h>
#include <ipc/ipcbuf.h>

void Util::panicArch() {
	/* at first, halt the other CPUs */
	SMP::haltOthers();

	/* enter vga-mode to be sure that the user can see the panic :) */
	/* actually it may fail depending on what caused the panic. this may make it more difficult
	 * to find the real reason for a failure. so it might be a good idea to turn it off during
	 * kernel-debugging :) */
	switchToVGA();
}

void Util::switchToVGA() {
	pid_t pid = Proc::getRunning();
	OpenFile *file;
	if(VFS::openPath(pid,VFS_MSGS | VFS_NOBLOCK,0,"/dev/vga",&file) == 0) {
		ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
		ipc::IPCBuf ib(buffer,sizeof(buffer));
		/* use an empty shm-name here. we don't need that anyway */
		ib << 3 << 1 << true << ipc::CString("");

		ssize_t res = file->sendMsg(pid,MSG_SCR_SETMODE,ib.buffer(),ib.pos(),NULL,0);
		if(res > 0) {
			for(int i = 0; i < 100; i++) {
				msgid_t mid = res;
				res = file->receiveMsg(pid,&mid,NULL,0,VFS_NOBLOCK);
				if(res >= 0)
					break;
				Thread::switchAway();
			}
		}
		file->close(pid);
	}
}
