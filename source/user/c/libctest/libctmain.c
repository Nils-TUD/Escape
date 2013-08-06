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

#include <esc/common.h>
#include <esc/proc.h>
#include <esc/test.h>
#include <stdlib.h>

#include "tests/theap.h"
#include "tests/tfileio.h"
#include "tests/tdir.h"
#include "tests/tenv.h"
#include "tests/tfs.h"
#include "tests/tgroup.h"
#include "tests/tuser.h"
#include "tests/trect.h"
#include "tests/tpasswd.h"
#include "tests/tmem.h"
#include "tests/tsllist.h"

int main(void) {
	if(getuid() != ROOT_UID)
		error("Please start this program as root!");

	test_register(&tModHeap);
	test_register(&tModFileio);
	test_register(&tModDir);
	test_register(&tModEnv);
	test_register(&tModFs);
	test_register(&tModGroup);
	test_register(&tModUser);
	test_register(&tModPasswd);
	test_register(&tModRect);
	test_register(&tModMem);
	test_register(&tModSLList);
	test_start();
	return EXIT_SUCCESS;
}
