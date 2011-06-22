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

#include <sys/common.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/task/proc.h>
#include <sys/mem/kheap.h>
#include <sys/video.h>
#include <esc/test.h>
#include <string.h>
#include <errors.h>
#include "tvfs.h"
#include "testutils.h"

/* forward declarations */
static void test_vfs(void);
static void test_vfs_createDriver(void);

/* our test-module */
sTestModule tModVFS = {
	"VFS",
	&test_vfs
};

static void test_vfs(void) {
	test_vfs_createDriver();
}

static void test_vfs_createDriver(void) {
	tFileNo f1,f2,f3;

	test_caseStart("Testing vfs_createDriver()");
	checkMemoryBefore(false);

	f1 = vfs_createDriver(0,"test",0);
	test_assertTrue(f1 >= 0);
	f2 = vfs_createDriver(0,"test2",0);
	test_assertTrue(f2 >= 0);
	test_assertInt(vfs_createDriver(1,"test",0),ERR_DRIVER_EXISTS);
	test_assertInt(vfs_createDriver(1,"",0),ERR_INV_DRIVER_NAME);
	test_assertInt(vfs_createDriver(1,"abc.def",0),ERR_INV_DRIVER_NAME);
	f3 = vfs_createDriver(1,"test3",0);
	test_assertTrue(f3 >= 0);

	vfs_closeFile(KERNEL_PID,f1);
	vfs_closeFile(KERNEL_PID,f2);
	vfs_closeFile(KERNEL_PID,f3);

	checkMemoryAfter(false);
	test_caseSucceeded();
}
