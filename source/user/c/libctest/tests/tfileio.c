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
#include <esc/test.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "tfileio.h"

/* forward declarations */
static void test_fileio(void);
static bool test_fileio_checkPrint(int recvRes,int expRes,char *recvStr,const char *expStr);
static bool test_fileio_checkScan(uint recvRes,uint expRes,const char *fmt,...);
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

static bool test_fileio_checkPrint(int res,int expRes,char *recvStr,const char *expStr) {
	if(!test_assertStr(recvStr,expStr))
		return false;
	if(!test_assertInt(res,expRes == -1 ? (int)strlen(expStr) : expRes))
		return false;
	return true;
}

static bool test_fileio_checkScan(uint recvRes,uint expRes,const char *fmt,...) {
	va_list ap;
	char ch;
	char *rs,*es;
	uint ru,eu;
	int rd,ed;

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
				rd = va_arg(ap,int);
				ed = va_arg(ap,int);
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
				ru = va_arg(ap,uint);
				eu = va_arg(ap,uint);
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
	char str[200];
	size_t i;
	int res;

	test_caseStart("Testing *printf()");

	res = snprintf(str,sizeof(str),"%d",4);
	if(!test_fileio_checkPrint(res,-1,str,"4"))
		return;

	res = snprintf(str,sizeof(str),"");
	if(!test_fileio_checkPrint(res,-1,str,""))
		return;

	res = snprintf(str,sizeof(str),"%i%d",123,456);
	if(!test_fileio_checkPrint(res,-1,str,"123456"))
		return;

	res = snprintf(str,sizeof(str),"_%d_%d_",1,2);
	if(!test_fileio_checkPrint(res,-1,str,"_1_2_"))
		return;

	res = snprintf(str,sizeof(str),"x=%x, X=%X, b=%b, o=%o, d=%d, u=%u",0xABC,0xDEF,0xF0F,0723,-675,412);
	if(!test_fileio_checkPrint(res,-1,str,"x=abc, X=DEF, b=111100001111, o=723, d=-675, u=412"))
		return;

	res = snprintf(str,sizeof(str),"'%s'_%c_","test",'f');
	if(!test_fileio_checkPrint(res,-1,str,"'test'_f_"))
		return;

	res = snprintf(str,sizeof(str),"%s","");
	if(!test_fileio_checkPrint(res,-1,str,""))
		return;

	res = snprintf(str,sizeof(str),"%s",NULL);
	if(!test_fileio_checkPrint(res,-1,str,"(null)"))
		return;

	res = snprintf(str,sizeof(str),"%.2s %8s",NULL,NULL);
	if(!test_fileio_checkPrint(res,-1,str,"(n   (null)"))
		return;

	res = snprintf(str,sizeof(str),"%10s","padme");
	if(!test_fileio_checkPrint(res,-1,str,"     padme"))
		return;

	res = snprintf(str,sizeof(str),"%02d, % 4x, %08b",9,0xff,0xf);
	if(!test_fileio_checkPrint(res,-1,str,"09,   ff, 00001111"))
		return;

	res = snprintf(str,sizeof(str),"%p%zn, %hx",0xdeadbeef,&i,0x12345678);
	if(sizeof(uintptr_t) == 4) {
		if(!test_fileio_checkPrint(res,-1,str,"dead:beef, 5678") || !test_assertSize(i,9))
			return;
	}
	else if(sizeof(uintptr_t) == 8) {
		if(!test_fileio_checkPrint(res,-1,str,"0000:0000:dead:beef, 5678") || !test_assertSize(i,9))
			return;
	}
	else
		test_assertFalse(true);

	res = snprintf(str,sizeof(str),"%Ld, %017Ld, %-*Ld",1LL,8167127123123123LL,12,-81273123LL);
	if(!test_fileio_checkPrint(res,-1,str,"1, 08167127123123123, -81273123   "))
		return;

	res = snprintf(str,sizeof(str),"%Lu, %017Lx, %#-*LX",1ULL,0x7179bafed2122ULL,12,0x1234ABULL);
	if(!test_fileio_checkPrint(res,-1,str,"1, 00007179bafed2122, 0X1234AB    "))
		return;

	res = snprintf(str,sizeof(str),"%f, %f, %f, %f, %f, %f",0.f,1.f,-1.f,0.f,0.4f,18.4f);
	if(!test_fileio_checkPrint(res,-1,str,"0.000000, 1.000000, -1.000000, 0.000000, 0.400000, 18.399999"))
		return;

	res = snprintf(str,sizeof(str),"%f, %f, %f, %f",-1.231f,999.999f,1234.5678f,1189378123.78167213123f);
	if(!test_fileio_checkPrint(res,-1,str,"-1.230999, 999.999023, 1234.567749, 1189378176.000000"))
		return;

	res = snprintf(str,sizeof(str),"%lf, %lf, %lf, %lf, %lf, %lf",0.,1.,-1.,0.,0.4,18.4);
	if(!test_fileio_checkPrint(res,-1,str,"0.000000, 1.000000, -1.000000, 0.000000, 0.400000, 18.399999"))
		return;

	res = snprintf(str,sizeof(str),"%lf, %lf, %lf, %lf",-1.231,999.999,1234.5678,1189378123.78167213123);
	if(!test_fileio_checkPrint(res,-1,str,"-1.231000, 999.999000, 1234.567800, 1189378123.781672"))
		return;

	res = snprintf(str,sizeof(str),"%8.4lf, %8.1lf, %8.10lf",1.,-1.,0.);
	if(!test_fileio_checkPrint(res,-1,str,"  1.0000,     -1.0, 0.0000000000"))
		return;

	res = snprintf(str,sizeof(str),"%3.0lf, %-06.1lf, %2.4lf, %10.10lf",-1.231,999.999,1234.5678,
			1189378123.78167213123);
	if(!test_fileio_checkPrint(res,-1,str,"-1., 999.90, 1234.5678, 1189378123.7816722393"))
		return;

	test_caseSucceeded();
}

static void test_fileio_scan(void) {
	char s[200],c1,c2,c3,c4;
	uint x,o,b,u;
	int d1,d2,d3;
	int res;
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

	test_caseSucceeded();
}
