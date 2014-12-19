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

#pragma once

#include <sys/common.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <fs/common.h>
#include <errno.h>
#include <utime.h>
#include <stdio.h>

/**
 * Utility functions for filesystem related permission checking
 */
class Permissions {
	Permissions() = delete;

public:
	static int canAccess(FSUser *u,mode_t mode,uid_t uid,gid_t gid,uint perms) {
		int mask;
		if(u->uid == ROOT_UID) {
			/* root has exec-permission if at least one has exec-permission */
			if(perms & MODE_EXEC)
				return (mode & MODE_EXEC) ? 0 : -EACCES;
			/* root can read and write in all cases */
			return 0;
		}

		if(uid == u->uid)
			mask = (mode >> 6) & 0x7;
		else if(gid == u->gid || isingroup(u->pid,gid) == 1)
			mask = (mode >> 3) & 0x7;
		else
			mask = mode & 0x7;

		if((perms & MODE_READ) && !(mask & MODE_READ))
			return -EACCES;
		if((perms & MODE_WRITE) && !(mask & MODE_WRITE))
			return -EACCES;
		if((perms & MODE_EXEC) && !(mask & MODE_EXEC))
			return -EACCES;
		return 0;
	}

	static bool canChmod(FSUser *u,uid_t uid) {
		/* root can chmod all files; otherwise it has to be the owner */
		if(u->uid != uid && u->uid != ROOT_UID)
			return false;
		return true;
	}

	static bool canUtime(FSUser *u,uid_t uid) {
		return canChmod(u,uid);
	}

	static bool canChown(FSUser *u,uid_t oldUid,gid_t oldGid,uid_t newUid,gid_t newGid) {
		/* root can chown everything; others can only chown their own files */
		if(u->uid != oldUid && u->uid != ROOT_UID)
			return false;
		if(u->uid != ROOT_UID) {
			/* users can't change the owner */
			if(newUid != (uid_t)-1 && newUid != oldUid && newUid != u->uid)
				return false;
			/* users can change the group only to a group they're a member of */
			if(newGid != (gid_t)-1 && newGid != oldGid && newGid != u->gid && !isingroup(u->pid,newGid))
				return false;
		}
		return true;
	}
};
