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

#include <common.h>
#include <col/treap.h>
#include <col/slist.h>
#include <vfs/fileid.h>

struct VMRegion;
class OpenFile;

class ShFiles {
	friend struct VMRegion;

	ShFiles() = delete;

	struct FileUsage : public SListItem {
		explicit FileUsage(pid_t pid,uintptr_t addr) : SListItem(), pid(pid), addr(addr) {
		}

		pid_t pid;
		uintptr_t addr;
	};

	struct FileNode : public TreapNode<FileId> {
		explicit FileNode(const FileId &id) : TreapNode<FileId>(id), usages() {
		}

		virtual void print(OStream &os) {
			os.writef("file=(%u,%u) with %zu usages\n",key().dev,key().ino,usages.length());
		}

		SList<FileUsage> usages;
	};

public:
	static int add(VMRegion *vmreg,pid_t pid);
	static bool get(OpenFile *f,uintptr_t *addr,pid_t *pid);
	static void remove(VMRegion *vmreg);

	static Treap<FileNode> tree;
	static SpinLock lock;
};
