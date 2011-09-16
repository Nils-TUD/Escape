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

#ifndef SYSCALLS_SIGNALS_H_
#define SYSCALLS_SIGNALS_H_

#include <sys/common.h>
#include <sys/task/thread.h>
#include <sys/intrpt.h>

int sysc_setSigHandler(sThread *t,sIntrptStackFrame *stack);
int sysc_ackSignal(sThread *t,sIntrptStackFrame *stack);
int sysc_sendSignalTo(sThread *t,sIntrptStackFrame *stack);

#endif /* SYSCALLS_SIGNALS_H_ */
