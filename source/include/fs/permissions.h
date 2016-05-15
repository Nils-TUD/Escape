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

#include <fs/common.h>
#include <sys/common.h>
#include <sys/stat.h>
#include <sys/proc.h>
#include <errno.h>

namespace fs {

/**
 * Utility functions for filesystem related permission checking
 */
class Permissions {
	Permissions() = delete;

public:
	template<int (*INGROUP)(pid_t,gid_t) = isingroup>
	static int canAccess(User *u,mode_t mode,uid_t uid,gid_t gid,uint perms) {
		int mask;
		if(u->uid == ROOT_UID) {
			/* root has exec-permission if at least one has exec-permission */
			if(perms & MODE_EXEC)
				return (mode & MODE_EXEC) ? 0 : -EACCES;
			/* root can read and write in all cases */
			return 0;
		}

		/* determine mask */
		if(uid == u->uid)
			mask = mode & S_IRWXU;
		else if(gid == u->gid || INGROUP(u->pid,gid) == 1)
			mask = mode & S_IRWXG;
		else
			mask = mode & S_IRWXO;

		/* check access */
		if((perms & MODE_READ) && !(mask & MODE_READ))
			return -EACCES;
		if((perms & MODE_WRITE) && !(mask & MODE_WRITE))
			return -EACCES;
		if((perms & MODE_EXEC) && !(mask & MODE_EXEC))
			return -EACCES;
		return 0;
	}

	static int canRemove(User *u,mode_t dirmode,uid_t diruid,uid_t fileuid) {
		/* if the sticky flag is set, we need to be owner of the dir or the file to unlink */
		if(dirmode & S_ISSTICKY) {
			if(u->uid != ROOT_UID && diruid != u->uid && fileuid != u->uid)
				return -EPERM;
		}
		return 0;
	}

	static bool canChmod(User *u,uid_t uid) {
		/* root can chmod all files; otherwise it has to be the owner */
		if(u->uid != uid && u->uid != ROOT_UID)
			return false;
		return true;
	}

	static bool canUtime(User *u,uid_t uid) {
		return canChmod(u,uid);
	}

	template<int (*INGROUP)(pid_t,gid_t) = isingroup>
	static bool canChown(User *u,uid_t oldUid,gid_t oldGid,uid_t newUid,gid_t newGid) {
		/* root can chown everything; others can only chown their own files */
		if(u->uid != oldUid && u->uid != ROOT_UID)
			return false;
		if(u->uid != ROOT_UID) {
			/* users can't change the owner */
			if(newUid != (uid_t)-1 && newUid != oldUid && newUid != u->uid)
				return false;
			/* users can change the group only to a group they're a member of */
			if(newGid != (gid_t)-1 && newGid != oldGid && newGid != u->gid && !INGROUP(u->pid,newGid))
				return false;
		}
		return true;
	}
};

}
