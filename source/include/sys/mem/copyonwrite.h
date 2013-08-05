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

#pragma once

#include <sys/common.h>
#include <sys/task/proc.h>
#include <sys/col/slist.h>

class CopyOnWrite {
	CopyOnWrite() = delete;

	struct Entry : public SListItem {
		explicit Entry(frameno_t frameNo) : SListItem(), frameNumber(frameNo), refCount(0) {
		}
		frameno_t frameNumber;
		size_t refCount;
	};

	static const size_t HEAP_SIZE	= 64;

public:
	/**
	 * Handles a pagefault for given address. Assumes that the pagefault was caused by a write access
	 * to a copy-on-write page!
	 *
	 * @param address the address
	 * @param frameNumber the frame for that address
	 * @return the number of frames that should be added to the current process
	 */
	static size_t pagefault(uintptr_t address,frameno_t frameNumber);

	/**
	 * Adds the given frame to the cow-list.
	 *
	 * @param frameNo the frame-number
	 * @return true if successfull
	 */
	static bool add(frameno_t frameNo);

	/**
	 * Removes the given frame from the cow-list
	 *
	 * @param frameNo the frame-number
	 * @param foundOther will be set to true if another process still uses the frame
	 * @return the number of frames to remove from <p>
	 */
	static size_t remove(frameno_t frameNo,bool *foundOther);

	/**
	 * Note that this is intended for debugging or similar only! (not very efficient)
	 *
	 * @return the number of different frames that are in the cow-list
	 */
	static size_t getFrmCount();

	/**
	 * Prints the cow-list
	 *
	 * @param os the output-stream
	 */
	static void print(OStream &os);

private:
	static Entry *getByFrame(frameno_t frameNo,bool dec);

	static SList<Entry> frames[];
	static klock_t lock;
};
