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

#include <common.h>
#include <vfs.h>
#include <vfsnode.h>
#include "tvfsnode.h"
#include <test.h>
#include <string.h>
#include <errors.h>

static void test_vfsn(void);
static void test_vfsn_resolvePath(void);
static bool test_vfsn_resolveRealPath(const char *a);
static bool test_vfsn_resolvePathCpy(const char *a,const char *b);
static void test_vfsn_getPath(void);

/* our test-module */
sTestModule tModVFSn = {
	"VFSNode",
	&test_vfsn
};

static void test_vfsn(void) {
	test_vfsn_resolvePath();
	test_vfsn_getPath();
}

static void test_vfsn_resolvePath(void) {
	test_caseStart("Testing vfsn_resolvePath()");

	if(!test_vfsn_resolvePathCpy("system:/..","system")) return;
	if(!test_vfsn_resolvePathCpy("system://../..","system")) return;
	if(!test_vfsn_resolvePathCpy("system://./.","system")) return;
	if(!test_vfsn_resolvePathCpy("system:/","system")) return;
	if(!test_vfsn_resolvePathCpy("system://","system")) return;
	if(!test_vfsn_resolvePathCpy("system:///","system")) return;
	if(!test_vfsn_resolvePathCpy("system:/processes/..","system")) return;
	if(!test_vfsn_resolvePathCpy("system:/processes/.","processes")) return;
	if(!test_vfsn_resolvePathCpy("system:/processes/./","processes")) return;
	if(!test_vfsn_resolvePathCpy("system:/./.","system")) return;
	if(!test_vfsn_resolvePathCpy("system://///processes/./././.","processes")) return;
	if(!test_vfsn_resolvePathCpy("system:/../processes/../processes/./","processes")) return;
	if(!test_vfsn_resolvePathCpy("system://..//..//..","system")) return;

	test_caseSucceded();
}

static bool test_vfsn_resolveRealPath(const char *a) {
	tVFSNodeNo no;
	return test_assertInt(vfsn_resolvePath(a,&no),ERR_REAL_PATH);
}

static bool test_vfsn_resolvePathCpy(const char *a,const char *b) {
	s32 err;
	tVFSNodeNo no;
	sVFSNode *node;
	if((err = vfsn_resolvePath(a,&no)) != 0) {
		test_caseFailed("Unable to resolve the path %s",a);
		return false;
	}

	node = vfsn_getNode(no);
	return test_assertStr(node->name,(char*)b);
}

static void test_vfsn_getPath(void) {
	tVFSNodeNo no;

	test_caseStart("Testing vfsn_getPath()");

	vfsn_resolvePath("system:",&no);
	test_assertStr(vfsn_getPath(no),(char*)"system:");

	vfsn_resolvePath("system:/processes",&no);
	test_assertStr(vfsn_getPath(no),(char*)"system:/processes");

	vfsn_resolvePath("system:/processes/0",&no);
	test_assertStr(vfsn_getPath(no),(char*)"system:/processes/0");

	test_caseSucceded();
}
