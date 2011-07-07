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

#ifndef CMDS_H_
#define CMDS_H_

#include <esc/common.h>
#include <esc/messages.h>
#include "mount.h"

sFSInst *cmds_getRoot(void);
void cmds_setRoot(dev_t dev,sFSInst *fs);
bool cmds_execute(int cmd,int fd,sMsg *msg,void *data);

#endif /* CMDS_H_ */
