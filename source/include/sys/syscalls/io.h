/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <sys/common.h>
#include <sys/task/thread.h>
#include <sys/intrpt.h>

int sysc_open(sThread *t,sIntrptStackFrame *stack);
int sysc_fcntl(sThread *t,sIntrptStackFrame *stack);
int sysc_pipe(sThread *t,sIntrptStackFrame *stack);
int sysc_tell(sThread *t,sIntrptStackFrame *stack);
int sysc_eof(sThread *t,sIntrptStackFrame *stack);
int sysc_seek(sThread *t,sIntrptStackFrame *stack);
int sysc_read(sThread *t,sIntrptStackFrame *stack);
int sysc_write(sThread *t,sIntrptStackFrame *stack);
int sysc_dup(sThread *t,sIntrptStackFrame *stack);
int sysc_redirect(sThread *t,sIntrptStackFrame *stack);
int sysc_close(sThread *t,sIntrptStackFrame *stack);
int sysc_send(sThread *t,sIntrptStackFrame *stack);
int sysc_receive(sThread *t,sIntrptStackFrame *stack);
int sysc_stat(sThread *t,sIntrptStackFrame *stack);
int sysc_fstat(sThread *t,sIntrptStackFrame *stack);
int sysc_chmod(sThread *t,sIntrptStackFrame *stack);
int sysc_chown(sThread *t,sIntrptStackFrame *stack);
int sysc_sync(sThread *t,sIntrptStackFrame *stack);
int sysc_link(sThread *t,sIntrptStackFrame *stack);
int sysc_unlink(sThread *t,sIntrptStackFrame *stack);
int sysc_mkdir(sThread *t,sIntrptStackFrame *stack);
int sysc_rmdir(sThread *t,sIntrptStackFrame *stack);
int sysc_mount(sThread *t,sIntrptStackFrame *stack);
int sysc_unmount(sThread *t,sIntrptStackFrame *stack);

#endif /* SYSCALLS_IO_H_ */

