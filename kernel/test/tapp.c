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
#include <app.h>
#include "tapp.h"
#include <test.h>

static void test_app(void);
static void test_app_1(void);
static void test_app_2(void);

static void test_checkRangeList(sRange *ranges,u32 count,sSLList *list);
static void test_checkIntList(u32 *ints,u32 count,sSLList *list);
static void test_checkDrvList(sDriverPerm *drivers,u32 count,sSLList *list);
static void test_checkStrList(const char **strings,u32 count,sSLList *list);

static const char *app1 =
	"source:				\"mysource\";"
	"sourceWritable:		0;"
	"type:					\"driver\";"
	"ioports:				123..123,124..150,1..4;"
	"driver:"
	"	\"null\",1,0,0,"
	"	\"BINPRIV\",1,1,1,"
	"	\"zero\",0,0,0;"
	"fs:					1,1;"
	"services:				\"env\";"
	"signals:				1,2,12;"
	"physmem:				4096..8191,0..3;"
	"crtshmem:				\"my\",\"my2\";"
	"joinshmem:				;";

static const char *app2 =
	"source:				\"hiho\";"
	"sourceWritable:		1;"
	"type:					\"default\";"
	"ioports:				;"
	"driver:;"
	"fs:					  0,  0  ;"
	"services:				\"env\",\"vesa\";"
	"signals:				;"
	"physmem:				;"
	"crtshmem:				\"aaaaabb\";"
	"joinshmem:				\"test\";";

/* our test-module */
sTestModule tModApp = {
	"App",
	&test_app
};

static void test_app(void) {
	test_app_1();
	test_app_2();
}

static void test_app_1(void) {
	char src[31];
	bool srcWritable;
	sRange ioports[] = {{123,123},{124,150},{1,4}};
	sRange physmem[] = {{4096,8191},{0,3}};
	const char *services[] = {"env"};
	const char *crtshmem[] = {"my","my2"};
	const char *joinshmem[0];
	u32 i,signals[] = {1,2,12};
	sDriverPerm driver[] = {
		{DRV_GROUP_CUSTOM,"null",1,0,0},
		{DRV_GROUP_BINPRIV,"",1,1,1},
		{DRV_GROUP_CUSTOM,"zero",0,0,0},
	};
	bool res;
	sApp a;
	sStringBuffer buf;
	buf.dynamic = true;
	buf.len = 0;
	buf.size = 0;
	buf.str = NULL;
	test_caseStart("Parsing, toString(), parsing again");

	res = app_fromString(app1,&a,src,&srcWritable);
	test_assertTrue(res);
	for(i = 0; i < 2; i++) {
		test_assertStr(src,"mysource");
		test_assertFalse(srcWritable);
		test_assertUInt(a.appType,APP_TYPE_DRIVER);
		test_checkRangeList(ioports,ARRAY_SIZE(ioports),a.ioports);
		test_checkDrvList(driver,ARRAY_SIZE(driver),a.driver);
		test_checkStrList(services,ARRAY_SIZE(services),a.services);
		test_checkIntList(signals,ARRAY_SIZE(signals),a.signals);
		test_checkRangeList(physmem,ARRAY_SIZE(physmem),a.physMem);
		test_checkStrList(crtshmem,ARRAY_SIZE(crtshmem),a.createShMem);
		test_checkStrList(joinshmem,ARRAY_SIZE(joinshmem),a.joinShMem);

		/* create str from it and parse again */
		if(i < 1) {
			test_assertTrue(app_toString(&buf,&a,src,srcWritable));
			test_assertTrue(app_fromString(buf.str,&a,src,&srcWritable));
		}
	}

	test_caseSucceded();
}

static void test_app_2(void) {
	char src[31];
	bool srcWritable;
	sRange ioports[0];
	sRange physmem[0];
	const char *services[] = {"env","vesa"};
	const char *crtshmem[] = {"aaaaabb"};
	const char *joinshmem[] = {"test"};
	u32 i,signals[0];
	sDriverPerm driver[0];
	bool res;
	sApp a;
	sStringBuffer buf;
	buf.dynamic = true;
	buf.len = 0;
	buf.size = 0;
	buf.str = NULL;
	test_caseStart("Parsing, toString(), parsing again");

	res = app_fromString(app2,&a,src,&srcWritable);
	test_assertTrue(res);
	for(i = 0; i < 2; i++) {
		test_assertStr(src,"hiho");
		test_assertTrue(srcWritable);
		test_assertUInt(a.appType,APP_TYPE_DEFAULT);
		test_checkRangeList(ioports,ARRAY_SIZE(ioports),a.ioports);
		test_checkDrvList(driver,ARRAY_SIZE(driver),a.driver);
		test_checkStrList(services,ARRAY_SIZE(services),a.services);
		test_checkIntList(signals,ARRAY_SIZE(signals),a.signals);
		test_checkRangeList(physmem,ARRAY_SIZE(physmem),a.physMem);
		test_checkStrList(crtshmem,ARRAY_SIZE(crtshmem),a.createShMem);
		test_checkStrList(joinshmem,ARRAY_SIZE(joinshmem),a.joinShMem);

		/* create str from it and parse again */
		if(i == 0) {
			test_assertTrue(app_toString(&buf,&a,src,srcWritable));
			test_assertTrue(app_fromString(buf.str,&a,src,&srcWritable));
		}
	}
	test_caseSucceded();
}

static void test_checkRangeList(sRange *ranges,u32 count,sSLList *list) {
	u32 i;
	sRange *r;
	if(!test_assertUInt(sll_length(list),count))
		return;
	for(i = 0; i < count; i++) {
		r = (sRange*)sll_get(list,i);
		test_assertUInt(r->start,ranges[i].start);
		test_assertUInt(r->end,ranges[i].end);
	}
}

static void test_checkIntList(u32 *ints,u32 count,sSLList *list) {
	u32 i;
	u32 val;
	if(!test_assertUInt(sll_length(list),count))
		return;
	for(i = 0; i < count; i++) {
		val = (u32)sll_get(list,i);
		test_assertUInt(val,ints[i]);
	}
}

static void test_checkDrvList(sDriverPerm *drivers,u32 count,sSLList *list) {
	u32 i;
	sDriverPerm *drv;
	if(!test_assertUInt(sll_length(list),count))
		return;
	for(i = 0; i < count; i++) {
		drv = (sDriverPerm*)sll_get(list,i);
		test_assertUInt(drv->group,drivers[i].group);
		test_assertStr(drv->name,drivers[i].name);
		test_assertTrue(drv->read == drivers[i].read);
		test_assertTrue(drv->write == drivers[i].write);
		test_assertTrue(drv->ioctrl == drivers[i].ioctrl);
	}
}

static void test_checkStrList(const char **strings,u32 count,sSLList *list) {
	u32 i;
	char *str;
	if(!test_assertUInt(sll_length(list),count))
		return;
	for(i = 0; i < count; i++) {
		str = (char*)sll_get(list,i);
		test_assertStr(str,strings[i]);
	}
}
