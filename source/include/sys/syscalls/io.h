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

#ifndef SYSCALLS_IO_H_
#define SYSCALLS_IO_H_

#include <sys/intrpt.h>

int sysc_open(sIntrptStackFrame *stack);
int sysc_fcntl(sIntrptStackFrame *stack);
int sysc_pipe(sIntrptStackFrame *stack);
int sysc_tell(sIntrptStackFrame *stack);
int sysc_eof(sIntrptStackFrame *stack);
int sysc_seek(sIntrptStackFrame *stack);
int sysc_read(sIntrptStackFrame *stack);
int sysc_write(sIntrptStackFrame *stack);
int sysc_dupFd(sIntrptStackFrame *stack);
int sysc_redirFd(sIntrptStackFrame *stack);
int sysc_close(sIntrptStackFrame *stack);
int sysc_send(sIntrptStackFrame *stack);
int sysc_receive(sIntrptStackFrame *stack);
int sysc_stat(sIntrptStackFrame *stack);
int sysc_fstat(sIntrptStackFrame *stack);
int sysc_chmod(sIntrptStackFrame *stack);
int sysc_chown(sIntrptStackFrame *stack);
int sysc_sync(sIntrptStackFrame *stack);
int sysc_link(sIntrptStackFrame *stack);
int sysc_unlink(sIntrptStackFrame *stack);
int sysc_mkdir(sIntrptStackFrame *stack);
int sysc_rmdir(sIntrptStackFrame *stack);
int sysc_mount(sIntrptStackFrame *stack);
int sysc_unmount(sIntrptStackFrame *stack);

#endif /* SYSCALLS_IO_H_ */

