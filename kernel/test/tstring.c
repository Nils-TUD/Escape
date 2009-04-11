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
#include <video.h>
#include <string.h>
#include <ctype.h>

#include "tstring.h"
#include <test.h>

/* forward declarations */
static void test_string(void);
static void test_atoi(void);
static void test_itoa(void);
static void test_memchr(void);
static void test_memcpy(void);
static void test_memcmp(void);
static void test_memmove(void);
static void test_strcpy(void);
static void test_strncpy(void);
static void test_strtok(void);
static void test_strcat(void);
static void test_strncat(void);
static void test_strcmp(void);
static void test_strncmp(void);
static void test_strchr(void);
static void test_strchri(void);
static void test_strrchr(void);
static void test_strstr(void);
static void test_strspn(void);
static void test_strcspn(void);
static void test_strpbrk(void);
static void test_strcut(void);
static void test_strlen(void);
static void test_strnlen(void);
static void test_tolower(void);
static void test_toupper(void);
static void test_isalnumstr(void);

/* our test-module */
sTestModule tModString = {
	"String",
	&test_string
};

static void test_string(void) {
	test_atoi();
	test_itoa();
	test_memchr();
	test_memcpy();
	test_memcmp();
	test_memmove();
	test_strcpy();
	test_strncpy();
	test_strtok();
	test_strcat();
	test_strncat();
	test_strcmp();
	test_strncmp();
	test_strchr();
	test_strchri();
	test_strrchr();
	test_strstr();
	test_strspn();
	test_strcspn();
	test_strpbrk();
	test_strcut();
	test_strlen();
	test_strnlen();
	test_tolower();
	test_toupper();
	test_isalnumstr();
}

static void test_atoi(void) {
	test_caseStart("Testing atoi()");

	if(!test_assertInt(atoi("123"),123)) return;
	if(!test_assertInt(atoi("1"),1)) return;
	if(!test_assertInt(atoi("0"),0)) return;
	if(!test_assertInt(atoi("123456789"),123456789)) return;
	if(!test_assertInt(atoi("-1"),-1)) return;
	if(!test_assertInt(atoi("-546"),-546)) return;
	if(!test_assertInt(atoi("  45  "),45)) return;
	if(!test_assertInt(atoi("\t\n\r12.4\n"),12)) return;
	if(!test_assertInt(atoi("2147483647"),2147483647)) return;
	if(!test_assertInt(atoi("-2147483648"),-2147483648)) return;
	if(!test_assertInt(atoi(""),0)) return;
	if(!test_assertInt(atoi("-"),0)) return;
	if(!test_assertInt(atoi("abc"),0)) return;
	if(!test_assertInt(atoi("a123b"),0)) return;

	test_caseSucceded();
}

static bool test_itoacpy(s32 n,const char *expected) {
	static char str[12];
	itoa(str,n);
	return test_assertStr(str,(char*)expected);
}

static void test_itoa(void) {
	test_caseStart("Testing itoa()");

	if(!test_itoacpy(1,"1")) return;
	if(!test_itoacpy(2,"2")) return;
	if(!test_itoacpy(8,"8")) return;
	if(!test_itoacpy(12,"12")) return;
	if(!test_itoacpy(7123,"7123")) return;
	if(!test_itoacpy(2147483647,"2147483647")) return;
	if(!test_itoacpy(0,"0")) return;
	if(!test_itoacpy(-1,"-1")) return;
	if(!test_itoacpy(-10,"-10")) return;
	if(!test_itoacpy(-8123,"-8123")) return;
	if(!test_itoacpy(-2147483648,"-2147483648")) return;

	test_caseSucceded();
}

static void test_memchr(void) {
	const char *s1 = "abc";
	const char *s2 = "def123456";
	test_caseStart("Testing memchr()");

	if(!test_assertPtr(memchr(s1,'a',3),(void*)s1)) return;
	if(!test_assertPtr(memchr(s1,'b',3),(void*)(s1 + 1))) return;
	if(!test_assertPtr(memchr(s1,'c',3),(void*)(s1 + 2))) return;
	if(!test_assertPtr(memchr(s1,'d',3),NULL)) return;
	if(!test_assertPtr(memchr(s2,'d',1),(void*)s2)) return;
	if(!test_assertPtr(memchr(s2,'e',1),NULL)) return;

	test_caseSucceded();
}

static void test_memcpy(void) {
	char src[11] = "0123456789",dest[11];
	test_caseStart("Testing memcpy()");

	memcpy(dest,src,10);
	dest[10] = '\0';
	if(!test_assertStr(dest,(char*)"0123456789")) return;
	memcpy(dest,src,4);
	dest[4] = '\0';
	if(!test_assertStr(dest,(char*)"0123")) return;
	memcpy(dest,src,0);
	dest[0] = '\0';
	if(!test_assertStr(dest,(char*)"")) return;

	test_caseSucceded();
}

static void test_memcmp(void) {
	const char *str1 = "abc", *str2 = "abcdef", *str3 = "def";
	test_caseStart("Testing memcmp()");

	if(!test_assertTrue(memcmp(str1,str2,3) == 0)) return;
	if(!test_assertTrue(memcmp(str1,str2,0) == 0)) return;
	if(!test_assertTrue(memcmp(str1,str2,1) == 0)) return;
	if(!test_assertTrue(memcmp(str2+3,str3,3) == 0)) return;
	if(!test_assertFalse(memcmp(str1,str2,4) == 0)) return;
	if(!test_assertFalse(memcmp(str2,str3,2) == 0)) return;

	test_caseSucceded();
}

static void test_memmove(void) {
	char str1[10] = "abc";
	char str2[10] = "def";
	char str3[10] = "";
	char dest[4];
	test_caseStart("Testing memmove()");

	if(!test_assertStr(memmove(dest,str1,4),(char*)"abc")) return;
	if(!test_assertStr(memmove(str1 + 1,str1,4),(char*)"abc") || !test_assertStr(str1,(char*)"aabc")) return;
	if(!test_assertStr(memmove(str2,str2 + 1,3),(char*)"ef") || !test_assertStr(str2,(char*)"ef")) return;
	if(!test_assertStr(memmove(str3,str3,0),(char*)"")) return;
	if(!test_assertStr(memmove(str3,"abcdef",7),(char*)"abcdef")) return;

	test_caseSucceded();
}

static void test_strcpy(void) {
	char target[20];
	test_caseStart("Testing strcpy()");

	if(!test_assertStr(strcpy(target,"abc"),(char*)"abc")) return;
	if(!test_assertStr(strcpy(target,""),(char*)"")) return;
	if(!test_assertStr(strcpy(target,"0123456789012345678"),(char*)"0123456789012345678")) return;
	if(!test_assertStr(strcpy(target,"123"),(char*)"123")) return;

	test_caseSucceded();
}

static void test_strncpy(void) {
	char target[20];
	char cmp1[] = {'a',0,0};
	char cmp2[] = {'d','e','f',0,0,0};
	char *str;
	test_caseStart("Testing strncpy()");

	str = strncpy(target,"abc",3);
	str[3] = '\0';
	if(!test_assertStr(str,(char*)"abc")) return;

	str = strncpy(target,"",0);
	str[0] = '\0';
	if(!test_assertStr(str,(char*)"")) return;

	str = strncpy(target,"0123456789012345678",19);
	str[19] = '\0';
	if(!test_assertStr(str,(char*)"0123456789012345678")) return;

	str = strncpy(target,"123",3);
	str[3] = '\0';
	if(!test_assertStr(str,(char*)"123")) return;

	str = strncpy(target,"abc",1);
	str[1] = '\0';
	if(!test_assertStr(str,(char*)"a")) return;

	str = strncpy(target,"abc",1);
	str[1] = '\0';
	if(!test_assertStr(str,cmp1)) return;

	str = strncpy(target,"defghi",3);
	str[3] = '\0';
	if(!test_assertStr(str,cmp2)) return;

	test_caseSucceded();
}

static void test_strtok(void) {
	char str1[] = "- This, a sample string.";
	char str2[] = "";
	char str3[] = ".,.";
	char str4[] = "abcdef";
	char str5[] = ",abc,def,";
	char str6[] = "abc,def";
	char *p;
	test_caseStart("Testing strtok()");

	/* str1 */
	p = strtok(str1," -,.");
	if(!test_assertStr(p,"This")) return;
	p = strtok(NULL," -,.");
	if(!test_assertStr(p,"a")) return;
	p = strtok(NULL," -,.");
	if(!test_assertStr(p,"sample")) return;
	p = strtok(NULL," -,.");
	if(!test_assertStr(p,"string")) return;
	p = strtok(NULL," -,.");
	if(!test_assertTrue(p == NULL)) return;

	/* str2 */
	p = strtok(str2," -,.");
	if(!test_assertTrue(p == NULL)) return;

	/* str3 */
	p = strtok(str3,".,");
	if(!test_assertTrue(p == NULL)) return;

	/* str4 */
	p = strtok(str4,"X");
	if(!test_assertStr(p,"abcdef")) return;
	p = strtok(NULL," -,.");
	if(!test_assertTrue(p == NULL)) return;

	/* str5 */
	p = strtok(str5,",");
	if(!test_assertStr(p,"abc")) return;
	p = strtok(NULL," -,.");
	if(!test_assertStr(p,"def")) return;
	p = strtok(NULL," -,.");
	if(!test_assertTrue(p == NULL)) return;

	/* str6 */
	p = strtok(str6,",");
	if(!test_assertStr(p,"abc")) return;
	p = strtok(NULL," -,.");
	if(!test_assertStr(p,"def")) return;
	p = strtok(NULL," -,.");
	if(!test_assertTrue(p == NULL)) return;

	test_caseSucceded();
}

static void test_strcat(void) {
	char str1[7] = "abc";
	char str2[] = "def";
	test_caseStart("Testing strcat()");

	if(!test_assertStr(strcat(str1,str2),(char*)"abcdef")) return;
	if(!test_assertStr(strcat(str1,""),(char*)"abcdef")) return;

	test_caseSucceded();
}

static void test_strncat(void) {
	char str1[10] = "abc";
	char str2[10] = "abc";
	char str3[10] = "abc";
	char str4[10] = "abc";
	char str5[10] = "abc";
	char str6[10] = "abc";
	char app[] = "def123";
	test_caseStart("Testing strnat()");

	if(!test_assertStr(strncat(str1,app,3),(char*)"abcdef")) return;
	if(!test_assertStr(strncat(str2,app,1),(char*)"abcd")) return;
	if(!test_assertStr(strncat(str3,app,0),(char*)"abc")) return;
	if(!test_assertStr(strncat(str4,app,4),(char*)"abcdef1")) return;
	if(!test_assertStr(strncat(str5,app,5),(char*)"abcdef12")) return;
	if(!test_assertStr(strncat(str6,app,6),(char*)"abcdef123")) return;

	test_caseSucceded();
}

static void test_strcmp(void) {
	test_caseStart("Testing strcmp()");

	if(!test_assertTrue(strcmp("","") == 0)) return;
	if(!test_assertTrue(strcmp("abc","abd") < 0)) return;
	if(!test_assertTrue(strcmp("abc","abb") > 0)) return;
	if(!test_assertTrue(strcmp("a","abc") < 0)) return;
	if(!test_assertTrue(strcmp("b","a") > 0)) return;
	if(!test_assertTrue(strcmp("1234","5678") < 0)) return;
	if(!test_assertTrue(strcmp("abc def","abc def") == 0)) return;
	if(!test_assertTrue(strcmp("123","123") == 0)) return;

	test_caseSucceded();
}

static void test_strncmp(void) {
	test_caseStart("Testing strncmp()");

	if(!test_assertTrue(strncmp("","",0) == 0)) return;
	if(!test_assertTrue(strncmp("abc","abd",3) < 0)) return;
	if(!test_assertTrue(strncmp("abc","abb",3) > 0)) return;
	if(!test_assertTrue(strncmp("a","abc",1) == 0)) return;
	if(!test_assertTrue(strncmp("b","a",1) > 0)) return;
	if(!test_assertTrue(strncmp("1234","5678",4) < 0)) return;
	if(!test_assertTrue(strncmp("abc def","abc def",7) == 0)) return;
	if(!test_assertTrue(strncmp("123","123",3) == 0)) return;
	if(!test_assertTrue(strncmp("ab","ab",1) == 0)) return;
	if(!test_assertTrue(strncmp("abc","abd",2) == 0)) return;
	if(!test_assertTrue(strncmp("abc","abd",0) == 0)) return;
	if(!test_assertTrue(strncmp("xyz","abc",3) > 0)) return;

	test_caseSucceded();
}

static void test_strchr(void) {
	char str1[] = "abcdef";
	test_caseStart("Testing strchr()");

	if(!test_assertTrue(strchr(str1,'a') == str1)) return;
	if(!test_assertTrue(strchr(str1,'b') == str1 + 1)) return;
	if(!test_assertTrue(strchr(str1,'c') == str1 + 2)) return;
	if(!test_assertTrue(strchr(str1,'g') == NULL)) return;

	test_caseSucceded();
}

static void test_strchri(void) {
	char str1[] = "abcdef";
	test_caseStart("Testing strchri()");

	if(!test_assertTrue(strchri(str1,'a') == 0)) return;
	if(!test_assertTrue(strchri(str1,'b') == 1)) return;
	if(!test_assertTrue(strchri(str1,'c') == 2)) return;
	if(!test_assertTrue(strchri(str1,'g') == (s32)strlen(str1))) return;

	test_caseSucceded();
}

static void test_strrchr(void) {
	char str1[] = "abcdefabc";
	test_caseStart("Testing strrchr()");

	if(!test_assertTrue(strrchr(str1,'a') == str1 + 6)) return;
	if(!test_assertTrue(strrchr(str1,'b') == str1 + 7)) return;
	if(!test_assertTrue(strrchr(str1,'c') == str1 + 8)) return;
	if(!test_assertTrue(strrchr(str1,'g') == NULL)) return;
	if(!test_assertTrue(strrchr(str1,'d') == str1 + 3)) return;

	test_caseSucceded();
}

static void test_strstr(void) {
	char str1[] = "abc def ghi";
	test_caseStart("Testing strstr()");

	if(!test_assertTrue(strstr(str1,"abc") == str1)) return;
	if(!test_assertTrue(strstr(str1,"def") == str1 + 4)) return;
	if(!test_assertTrue(strstr(str1,"ghi") == str1 + 8)) return;
	if(!test_assertTrue(strstr(str1,"a") == str1)) return;
	if(!test_assertTrue(strstr(str1,"abc def ghi") == str1)) return;
	if(!test_assertTrue(strstr(str1,"abc ghi") == NULL)) return;
	if(!test_assertTrue(strstr(str1,"") == NULL)) return;
	if(!test_assertTrue(strstr(str1,"def ghi") == str1 + 4)) return;
	if(!test_assertTrue(strstr(str1,"g") == str1 + 8)) return;
	if(!test_assertTrue(strstr("","abc") == NULL)) return;
	if(!test_assertTrue(strstr("","") == NULL)) return;

	test_caseSucceded();
}

static void test_strspn(void) {
	test_caseStart("Testing strspn()");

	if(!test_assertUInt(strspn("abc","def"),0)) return;
	if(!test_assertUInt(strspn("abc","a"),1)) return;
	if(!test_assertUInt(strspn("abc","ab"),2)) return;
	if(!test_assertUInt(strspn("abc","abc"),3)) return;
	if(!test_assertUInt(strspn("abc","bca"),3)) return;
	if(!test_assertUInt(strspn("abc","cab"),3)) return;
	if(!test_assertUInt(strspn("abc","cba"),3)) return;
	if(!test_assertUInt(strspn("abc","bac"),3)) return;
	if(!test_assertUInt(strspn("abc","b"),0)) return;
	if(!test_assertUInt(strspn("abc","cdef"),0)) return;
	if(!test_assertUInt(strspn("","123"),0)) return;
	if(!test_assertUInt(strspn("",""),0)) return;

	test_caseSucceded();
}

static void test_strcspn(void) {
	test_caseStart("Testing strcspn()");

	if(!test_assertUInt(strcspn("abc","def"),3)) return;
	if(!test_assertUInt(strcspn("abc","a"),0)) return;
	if(!test_assertUInt(strcspn("abc","b"),1)) return;
	if(!test_assertUInt(strcspn("abc","cdef"),2)) return;
	if(!test_assertUInt(strcspn("","123"),0)) return;
	if(!test_assertUInt(strcspn("",""),0)) return;

	test_caseSucceded();
}

static void test_strpbrk(void) {
	char str1[] = "test";
	char str2[] = "abc_def";
	char str3[] = "";
	test_caseStart("Testing strpbrk()");

	if(!test_assertStr(strpbrk(str1,"test"),str1)) return;
	if(!test_assertStr(strpbrk(str1,"t"),str1)) return;
	if(!test_assertStr(strpbrk(str1,"e"),str1 + 1)) return;
	if(!test_assertStr(strpbrk(str1,"s"),str1 + 2)) return;
	if(!test_assertStr(strpbrk(str2,"fde"),str2 + 4)) return;
	if(!test_assertStr(strpbrk(str2,"def"),str2 + 4)) return;
	if(!test_assertStr(strpbrk(str2,"fed"),str2 + 4)) return;
	if(!test_assertStr(strpbrk(str2,"__"),str2 + 3)) return;
	if(!test_assertTrue(strpbrk(str2,"ghxyz") == NULL)) return;
	if(!test_assertTrue(strpbrk(str3,"a") == NULL)) return;
	if(!test_assertTrue(strpbrk(str3,"xyz") == NULL)) return;
	if(!test_assertTrue(strpbrk(str3,"") == NULL)) return;
	if(!test_assertTrue(strpbrk(str2,"") == NULL)) return;
	if(!test_assertTrue(strpbrk(str1,"") == NULL)) return;

	test_caseSucceded();
}

static void test_strcut(void) {
	char str1[] = "abc def ghi";
	char str2[] = "abc";
	char str3[] = "a";
	char str4[] = "";
	char str5[] = "123456";
	test_caseStart("Testing strcut()");

	if(!test_assertStr(strcut(str1,4),(char*)"def ghi")) return;
	if(!test_assertStr(strcut(str2,3),(char*)"")) return;
	if(!test_assertStr(strcut(str3,0),(char*)"a")) return;
	if(!test_assertStr(strcut(str4,0),(char*)"")) return;
	if(!test_assertStr(strcut(str5,5),(char*)"6")) return;

	test_caseSucceded();
}

static void test_strlen(void) {
	test_caseStart("Testing strlen()");

	if(!test_assertUInt(strlen((char*)"abc"),3)) return;
	if(!test_assertUInt(strlen((char*)""),0)) return;
	if(!test_assertUInt(strlen((char*)"abcdef"),6)) return;
	if(!test_assertUInt(strlen((char*)"1"),1)) return;

	test_caseSucceded();
}

static void test_strnlen(void) {
	test_caseStart("Testing strnlen()");

	if(!test_assertInt(strnlen((char*)"abc",10),3)) return;
	if(!test_assertInt(strnlen((char*)"",10),0)) return;
	if(!test_assertInt(strnlen((char*)"abcdef",10),6)) return;
	if(!test_assertInt(strnlen((char*)"1",10),1)) return;
	if(!test_assertInt(strnlen((char*)"abc",3),3)) return;
	if(!test_assertInt(strnlen((char*)"",0),0)) return;
	if(!test_assertInt(strnlen((char*)"abcdef",6),6)) return;
	if(!test_assertInt(strnlen((char*)"1",1),1)) return;
	if(!test_assertInt(strnlen((char*)"abc",2),-1)) return;
	if(!test_assertInt(strnlen((char*)"abcdef",5),-1)) return;
	if(!test_assertInt(strnlen((char*)"1",0),-1)) return;

	test_caseSucceded();
}

static void test_tolower(void) {
	test_caseStart("Testing tolower()");

	if(!test_assertInt(tolower('C'),'c')) return;
	if(!test_assertInt(tolower('A'),'a')) return;
	if(!test_assertInt(tolower('Z'),'z')) return;
	if(!test_assertInt(tolower('a'),'a')) return;
	if(!test_assertInt(tolower('x'),'x')) return;
	if(!test_assertInt(tolower('z'),'z')) return;

	test_caseSucceded();
}

static void test_toupper(void) {
	test_caseStart("Testing toupper()");

	if(!test_assertInt(toupper('c'),'C')) return;
	if(!test_assertInt(toupper('a'),'A')) return;
	if(!test_assertInt(toupper('z'),'Z')) return;
	if(!test_assertInt(toupper('A'),'A')) return;
	if(!test_assertInt(toupper('X'),'X')) return;
	if(!test_assertInt(toupper('Z'),'Z')) return;

	test_caseSucceded();
}

static void test_isalnumstr(void) {
	test_caseStart("Testing isalnumstr()");

	if(!test_assertTrue(isalnumstr("abc123"))) return;
	if(!test_assertTrue(isalnumstr("Axer123084"))) return;
	if(!test_assertTrue(isalnumstr(""))) return;
	if(!test_assertTrue(isalnumstr("a"))) return;
	if(!test_assertTrue(isalnumstr("0"))) return;
	if(!test_assertFalse(isalnumstr("_"))) return;
	if(!test_assertFalse(isalnumstr("abc123*"))) return;
	if(!test_assertFalse(isalnumstr("../*/\\"))) return;
	if(!test_assertFalse(isalnumstr("12.5"))) return;

	test_caseSucceded();
}
