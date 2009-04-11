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
#include <esc/fileio.h>
#include <test.h>
#include <stdarg.h>
#include <string.h>
#include "tfileio.h"

/* forward declarations */
static void test_fileio(void);
static bool test_fileio_checkPrint(u32 recvRes,char *recvStr,const char *expStr);
static bool test_fileio_checkScan(u32 recvRes,u32 expRes,const char *fmt,...);
static void test_fileio_print(void);
static void test_fileio_scan(void);

/* our test-module */
sTestModule tModFileio = {
	"File-IO",
	&test_fileio
};

static void test_fileio(void) {
	test_fileio_print();
	test_fileio_scan();
}

static bool test_fileio_checkPrint(u32 res,char *recvStr,const char *expStr) {
	if(!test_assertStr(recvStr,expStr))
		return false;
	if(!test_assertUInt(res,strlen(expStr)))
		return false;
	return true;
}

static bool test_fileio_checkScan(u32 recvRes,u32 expRes,const char *fmt,...) {
	va_list ap;
	char ch;
	char *rs,*es;
	u32 ru,eu;
	s32 rd,ed;

	if(!test_assertUInt(recvRes,expRes))
		return false;

	va_start(ap,fmt);
	while((ch = *fmt++)) {
		if(ch == '%')
			ch = *fmt++;
		switch(ch) {
			/* signed */
			case 'c':
			case 'd':
				rd = va_arg(ap,s32);
				ed = va_arg(ap,s32);
				if(!test_assertInt(rd,ed)) {
					va_end(ap);
					return false;
				}
				break;

			/* unsigned */
			case 'x':
			case 'b':
			case 'o':
			case 'u':
				ru = va_arg(ap,u32);
				eu = va_arg(ap,u32);
				if(!test_assertUInt(ru,eu)) {
					va_end(ap);
					return false;
				}
				break;

			/* string */
			case 's':
				rs = va_arg(ap,char*);
				es = va_arg(ap,char*);
				if(!test_assertStr(rs,es)) {
					va_end(ap);
					return false;
				}
				break;
		}
	}
	va_end(ap);
	return true;
}

static void test_fileio_print(void) {
	char str[200],tmp[200],c;
	u32 res,i;

	test_caseStart("Testing *printf()");

	res = sprintf(str,"%d",4);
	if(!test_fileio_checkPrint(res,str,"4"))
		return;

	res = sprintf(str,"");
	if(!test_fileio_checkPrint(res,str,""))
		return;

	res = sprintf(str,"%i%d",123,456);
	if(!test_fileio_checkPrint(res,str,"123456"))
		return;

	res = sprintf(str,"_%d_%d_",1,2);
	if(!test_fileio_checkPrint(res,str,"_1_2_"))
		return;

	res = sprintf(str,"x=%x, X=%X, b=%b, o=%o, d=%d, u=%u",0xABC,0xDEF,0xF0F,0723,-675,412);
	if(!test_fileio_checkPrint(res,str,"x=abc, X=DEF, b=111100001111, o=723, d=-675, u=412"))
		return;

	res = sprintf(str,"'%s'_%c_","test",'f');
	if(!test_fileio_checkPrint(res,str,"'test'_f_"))
		return;

	res = sprintf(str,"%s","");
	if(!test_fileio_checkPrint(res,str,""))
		return;

	res = sprintf(str,"%10s","padme");
	if(!test_fileio_checkPrint(res,str,"     padme"))
		return;

	res = sprintf(str,"%02d, % 4x, %08b",9,0xff,0xf);
	if(!test_fileio_checkPrint(res,str,"09,   ff, 00001111"))
		return;

	strcpy(tmp,"09,   ff, AB , 0x12, 0X0AB,   +12,  13, 00000123, 00001111");
	for(i = 0; i <= strlen(tmp); i++) {
		c = tmp[i];
		tmp[i] = '\0';
		res = snprintf(str,i,"%02d, %4x, %-3X, %#2x, %#03X, %+5d, % 3d, %0*x, %08b",9,0xff,0xab,0x12,
				0xAB,12,13,8,0x123,0xf);
		if(!test_fileio_checkPrint(res,str,tmp))
			return;
		tmp[i] = c;
	}

	res = sprintf(str,"%p%n, %hx",0xdeadbeef,&i,0x12345678);
	if(!test_fileio_checkPrint(res,str,"dead:beef, 5678") || !test_assertUInt(i,9))
		return;

	test_caseSucceded();
}

static void test_fileio_scan(void) {
	char s[200],c1,c2,c3,c4;
	u32 x,o,b,u;
	s32 d1,d2,d3;
	u32 res;
	test_caseStart("Testing *scanf()");

	res = sscanf("ABC, 123","%x,%s",&x,s);
	if(!test_fileio_checkScan(res,2,"%x%s",x,0xABC,s,"123"))
		return;

	res = sscanf("test","");
	if(!test_fileio_checkScan(res,0,""))
		return;

	res = sscanf("x=deadBEEF, b=0011001, o=7124, d=-1023, u=748","x=%X, b=%b, o=%o, d=%d, u=%u",
			&x,&b,&o,&d1,&u);
	if(!test_fileio_checkScan(res,5,"%x%b%o%d%u",x,0xdeadbeef,b,0x19,o,07124,d1,-1023,u,748))
		return;

	res = sscanf("test","%c%c%c%c",&c1,&c2,&c3,&c4);
	if(!test_fileio_checkScan(res,4,"%c%c%c%c",c1,'t',c2,'e',c3,'s',c4,'t'))
		return;

	res = sscanf("str","%s",s);
	if(!test_fileio_checkScan(res,1,"%s",s,"str"))
		return;

	res = sscanf("str","%2s",s);
	if(!test_fileio_checkScan(res,1,"%s",s,"st"))
		return;

	res = sscanf("4/26/2009","%2d/%2d/%4d",&d1,&d2,&d3);
	if(!test_fileio_checkScan(res,3,"%d%d%d",d1,4,d2,26,d3,2009))
		return;

	res = sscanf("1234/5678/90123","%2d/%2d/%4d",&d1,&d2,&d3);
	if(!test_fileio_checkScan(res,1,"%d",d1,12))
		return;

	test_caseSucceded();
}
