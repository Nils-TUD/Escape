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

#include <esc/common.h>
#include <esc/exceptions/io.h>
#include <esc/io/file.h>
#include <test.h>
#include <string.h>
#include <stdlib.h>
#include "tfile.h"

/* forward declarations */
static void test_file(void);

/* our test-module */
sTestModule tModFile = {
	"File",
	&test_file
};

static void test_file(void) {
	u32 before;
	s32 res;
	bool ex;
	char buffer[MAX_PATH_LEN];
	sFile *f;
	test_caseStart("Testing creation & destroy & paths");

	before = heapspace();
	f = file_get("bin/libesctest");
	res = f->getAbsolute(f,buffer,sizeof(buffer));
	test_assertUInt(res,strlen(buffer));
	test_assertStr(buffer,"/bin/libesctest");
	res = f->getName(f,buffer,sizeof(buffer));
	test_assertUInt(res,strlen(buffer));
	test_assertStr(buffer,"libesctest");
	res = f->getParent(f,buffer,sizeof(buffer));
	test_assertUInt(res,strlen(buffer));
	test_assertStr(buffer,"/bin");
	f->destroy(f);
	test_assertUInt(heapspace(),before);

	ex = false;
	before = heapspace();
	TRY {
		f = file_get("myexoticpath");
	}
	CATCH(IOException,e) {
		ex = true;
	}
	ENDCATCH
	test_assertTrue(ex);
	f->destroy(f);
	test_assertUInt(heapspace(),before);

	before = heapspace();
	f = file_get("/system/processes/0/regions");
	res = f->getAbsolute(f,buffer,sizeof(buffer));
	test_assertUInt(res,strlen(buffer));
	test_assertStr(buffer,"/system/processes/0/regions");
	res = f->getName(f,buffer,sizeof(buffer));
	test_assertUInt(res,strlen(buffer));
	test_assertStr(buffer,"regions");
	res = f->getParent(f,buffer,sizeof(buffer));
	test_assertUInt(res,strlen(buffer));
	test_assertStr(buffer,"/system/processes/0");
	f->destroy(f);
	test_assertUInt(heapspace(),before);

	before = heapspace();
	f = file_get("/");
	res = f->getAbsolute(f,buffer,sizeof(buffer));
	test_assertUInt(res,strlen(buffer));
	test_assertStr(buffer,"/");
	res = f->getName(f,buffer,sizeof(buffer));
	test_assertUInt(res,strlen(buffer));
	test_assertStr(buffer,"");
	res = f->getParent(f,buffer,sizeof(buffer));
	test_assertUInt(res,strlen(buffer));
	test_assertStr(buffer,"/");
	f->destroy(f);
	test_assertUInt(heapspace(),before);

	test_caseSucceded();
}
