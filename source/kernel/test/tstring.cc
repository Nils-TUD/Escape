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

#include <sys/test.h>
#include <common.h>
#include <ctype.h>
#include <string.h>
#include <video.h>

/* forward declarations */
static void test_string();
static void test_atoi();
static void test_atoll();
static void test_itoa();
static void test_memset();
static void test_memclear();
static void test_memchr();
static void test_memcpy();
static void test_memcmp();
static void test_memmove();
static void test_strcpy();
static void test_strncpy();
static void test_strtok();
static void test_strcat();
static void test_strncat();
static void test_strcmp();
static void test_strncmp();
static void test_strcasecmp();
static void test_strncasecmp();
static void test_strchr();
static void test_strchri();
static void test_strrchr();
static void test_strstr();
static void test_strcasestr();
static void test_strspn();
static void test_strcspn();
static void test_strpbrk();
static void test_strcut();
static void test_strlen();
static void test_strnlen();
static void test_tolower();
static void test_toupper();
static void test_isalnumstr();
static void test_strmatch();
static void test_strtol();

/* our test-module */
sTestModule tModString = {
	"String",
	&test_string
};

static void test_string() {
	test_atoi();
	test_atoll();
	test_itoa();
	test_memset();
	test_memclear();
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
	test_strcasecmp();
	test_strncasecmp();
	test_strchr();
	test_strchri();
	test_strrchr();
	test_strstr();
	test_strcasestr();
	test_strspn();
	test_strcspn();
	test_strpbrk();
	test_strcut();
	test_strlen();
	test_strnlen();
	test_tolower();
	test_toupper();
	test_isalnumstr();
	test_strmatch();
	test_strtol();
}

static void test_atoi() {
	test_caseStart("Testing atoi()");

	test_assertInt(atoi("123"),123);
	test_assertInt(atoi("1"),1);
	test_assertInt(atoi("0"),0);
	test_assertInt(atoi("123456789"),123456789);
	test_assertInt(atoi("-1"),-1);
	test_assertInt(atoi("-546"),-546);
	test_assertInt(atoi("  45  "),45);
	test_assertInt(atoi("\t\n\r12.4\n"),12);
	test_assertInt(atoi("2147483647"),2147483647LL);
	test_assertInt(atoi("-2147483648"),-2147483648LL);
	test_assertInt(atoi(""),0);
	test_assertInt(atoi("-"),0);
	test_assertInt(atoi("abc"),0);
	test_assertInt(atoi("a123b"),0);

	test_caseSucceeded();
}

static void test_atoll() {
	test_caseStart("Testing atoll()");

	test_assertLLInt(atoll("123"),123);
	test_assertLLInt(atoll("1"),1);
	test_assertLLInt(atoll("0"),0);
	test_assertLLInt(atoll("123456789"),123456789);
	test_assertLLInt(atoll("-1"),-1);
	test_assertLLInt(atoll("-546"),-546);
	test_assertLLInt(atoll("  45  "),45);
	test_assertLLInt(atoll("\t\n\r12.4\n"),12);
	test_assertLLInt(atoll("2147483647"),2147483647LL);
	test_assertLLInt(atoll("-2147483648"),-2147483648LL);
	test_assertLLInt(atoll("8278217191231"),8278217191231LL);
	test_assertLLInt(atoll("-8278217191231"),-8278217191231LL);
	test_assertLLInt(atoll("9223372036854775807"),9223372036854775807LL);
	test_assertLLInt(atoll("-9223372036854775808"),-9223372036854775808LL);
	test_assertLLInt(atoll(""),0);
	test_assertLLInt(atoll("-"),0);
	test_assertLLInt(atoll("abc"),0);
	test_assertLLInt(atoll("a123b"),0);

	test_caseSucceeded();
}

static bool test_itoacpy(int n,const char *expected) {
	static char str[12];
	itoa(str,sizeof(str),n);
	return test_assertStr(str,(char*)expected);
}

static void test_itoa() {
	test_caseStart("Testing itoa()");

	test_itoacpy(1,"1");
	test_itoacpy(2,"2");
	test_itoacpy(8,"8");
	test_itoacpy(12,"12");
	test_itoacpy(7123,"7123");
	test_itoacpy(2147483647LL,"2147483647");
	test_itoacpy(0,"0");
	test_itoacpy(-1,"-1");
	test_itoacpy(-10,"-10");
	test_itoacpy(-8123,"-8123");
	test_itoacpy(-2147483648LL,"-2147483648");

	test_caseSucceeded();
}

static void checkZero(uint8_t *buf,size_t start,size_t len,size_t total) {
	while(start-- > 0) {
		if(*buf != 0xFF)
			test_assertUInt(*buf,0xFF);
		buf++;
		total--;
	}
	while(len-- > 0) {
		if(*buf != 0)
			test_assertUInt(*buf,0);
		buf++;
		total--;
	}
	while(total-- > 0) {
		if(*buf != 0xFF)
			test_assertUInt(*buf,0xFF);
		buf++;
	}
}

static void test_memset() {
	int i,j;
	uint8_t buf[16] = {0};
	test_caseStart("Testing memset()");

	memset(buf,' ',10);
	test_assertStr((char*)buf,"          ");
	memset(buf,'a',10);
	test_assertStr((char*)buf,"aaaaaaaaaa");
	memset(buf,'a',1);
	memset(buf + 1,'b',2);
	memset(buf + 3,'c',3);
	memset(buf + 6,'d',4);
	test_assertStr((char*)buf,"abbcccdddd");
	memset(buf,'X',0);
	test_assertStr((char*)buf,"abbcccdddd");
	memset(buf,'X',5);
	test_assertStr((char*)buf,"XXXXXcdddd");
	memset(buf,'X',6);
	test_assertStr((char*)buf,"XXXXXXdddd");
	memset(buf,'X',7);
	test_assertStr((char*)buf,"XXXXXXXddd");
	memset(buf,'X',8);
	test_assertStr((char*)buf,"XXXXXXXXdd");

	/* test alignment */
	for(i = 0; i < 8; i++) {
		for(j = 0; j < 8; j++) {
			memset(buf,0xFF,16);
			memset(buf + i,0,j);
			checkZero(buf,i,j,16);
		}
	}

	test_caseSucceeded();
}

static void test_memclear() {
	int i,j;
	uint8_t buf[16] = {0};
	test_caseStart("Testing memclear()");

	memset(buf,0xFF,16);
	memclear(buf,16);
	checkZero(buf,0,16,16);

	memset(buf,0xFF,16);
	memclear(buf,0);
	checkZero(buf,0,0,16);

	memset(buf,0xFF,16);
	memclear(buf,4);
	checkZero(buf,0,4,16);

	/* test alignment */
	for(i = 0; i < 8; i++) {
		for(j = 0; j < 8; j++) {
			memset(buf,0xFF,16);
			memclear(buf + i,j);
			checkZero(buf,i,j,16);
		}
	}

	test_caseSucceeded();
}

static void test_memcpy() {
	int i,j;
	uint8_t zeros[17] = {0};
	uint8_t src[17] = "0123456789ABCDEF",dest[17];
	test_caseStart("Testing memcpy()");

	test_assertPtr(memcpy(dest,src,10),dest);
	dest[10] = '\0';
	test_assertStr((char*)dest,"0123456789");
	test_assertPtr(memcpy(dest,src,4),dest);
	dest[4] = '\0';
	test_assertStr((char*)dest,"0123");
	test_assertPtr(memcpy(dest,src,0),dest);
	dest[0] = '\0';
	test_assertStr((char*)dest,"");

	/* test alignment */
	for(i = 0; i < 8; i++) {
		for(j = 0; j < 8; j++) {
			memset(dest,0xFF,sizeof(dest));
			test_assertPtr(memcpy(dest + i,zeros + i,j),dest + i);
			checkZero(dest,i,j,sizeof(dest));
		}
	}

	test_caseSucceeded();
}

static int abs(int n) {
	return n < 0 ? -n : n;
}

static void test_memmove() {
	int i,j;
	uint8_t zdest[32];
	char str1[10] = "abc";
	char str2[10] = "def";
	char str3[10] = "";
	char dest[4];
	test_caseStart("Testing memmove()");

	test_assertStr((char*)memmove(dest,str1,4),"abc");
	test_assertStr((char*)memmove(str1 + 1,str1,4),"abc") || !test_assertStr(str1,"aabc");
	test_assertStr((char*)memmove(str2,str2 + 1,3),"ef") || !test_assertStr(str2,"ef");
	test_assertStr((char*)memmove(str3,str3,0),"");
	test_assertStr((char*)memmove(str3,"abcdef",7),"abcdef");

	/* test alignment */
	for(i = 0; i < 8; i++) {
		for(j = 0; j < 8; j++) {
			memset(zdest,0xFF,32);
			memset(zdest + j,0,abs(i - j));
			memmove(zdest + i,zdest + j,abs(i - j));
			checkZero(zdest,MIN(i,j),abs(i - j) * 2,16);
		}
	}

	test_caseSucceeded();
}

static void test_memchr() {
	const char *s1 = "abc";
	const char *s2 = "def123456";
	test_caseStart("Testing memchr()");

	test_assertPtr(memchr(s1,'a',3),(void*)s1);
	test_assertPtr(memchr(s1,'b',3),(void*)(s1 + 1));
	test_assertPtr(memchr(s1,'c',3),(void*)(s1 + 2));
	test_assertPtr(memchr(s1,'d',3),NULL);
	test_assertPtr(memchr(s2,'d',1),(void*)s2);
	test_assertPtr(memchr(s2,'e',1),NULL);

	test_caseSucceeded();
}

static void test_memcmp() {
	const char *str1 = "abc", *str2 = "abcdef", *str3 = "def";
	test_caseStart("Testing memcmp()");

	test_assertTrue(memcmp(str1,str2,3) == 0);
	test_assertTrue(memcmp(str1,str2,0) == 0);
	test_assertTrue(memcmp(str1,str2,1) == 0);
	test_assertTrue(memcmp(str2+3,str3,3) == 0);
	test_assertFalse(memcmp(str1,str2,4) == 0);
	test_assertFalse(memcmp(str2,str3,2) == 0);

	test_caseSucceeded();
}

static void test_strcpy() {
	char target[20];
	test_caseStart("Testing strcpy()");

	test_assertStr(strcpy(target,"abc"),"abc");
	test_assertStr(strcpy(target,""),"");
	test_assertStr(strcpy(target,"0123456789012345678"),"0123456789012345678");
	test_assertStr(strcpy(target,"123"),"123");

	test_caseSucceeded();
}

static void test_strncpy() {
	char target[20];
	char cmp1[] = {'a',0,0};
	char cmp2[] = {'d','e','f',0,0,0};
	char *str;
	test_caseStart("Testing strncpy()");

	str = strncpy(target,"abc",3);
	str[3] = '\0';
	test_assertStr(str,"abc");

	str = strncpy(target,"",0);
	str[0] = '\0';
	test_assertStr(str,"");

	str = strncpy(target,"0123456789012345678",19);
	str[19] = '\0';
	test_assertStr(str,"0123456789012345678");

	str = strncpy(target,"123",3);
	str[3] = '\0';
	test_assertStr(str,"123");

	str = strncpy(target,"abc",1);
	str[1] = '\0';
	test_assertStr(str,"a");

	str = strncpy(target,"abc",1);
	str[1] = '\0';
	test_assertStr(str,cmp1);

	str = strncpy(target,"defghi",3);
	str[3] = '\0';
	test_assertStr(str,cmp2);

	test_caseSucceeded();
}

static void test_strtok() {
	char str1[] = "- This, a sample string.";
	char str2[] = "";
	char str3[] = ".,.";
	char str4[] = "abcdef";
	char str5[] = ",abc,def,";
	char str6[] = "abc,def";
	char str7[] = "?a???b,,,#c";
	char *p;
	test_caseStart("Testing strtok()");

	/* str1 */
	p = strtok(str1," -,.");
	test_assertStr(p,"This");
	p = strtok(NULL," -,.");
	test_assertStr(p,"a");
	p = strtok(NULL," -,.");
	test_assertStr(p,"sample");
	p = strtok(NULL," -,.");
	test_assertStr(p,"string");
	p = strtok(NULL," -,.");
	test_assertTrue(p == NULL);

	/* str2 */
	p = strtok(str2," -,.");
	test_assertTrue(p == NULL);

	/* str3 */
	p = strtok(str3,".,");
	test_assertTrue(p == NULL);

	/* str4 */
	p = strtok(str4,"X");
	test_assertStr(p,"abcdef");
	p = strtok(NULL," -,.");
	test_assertTrue(p == NULL);

	/* str5 */
	p = strtok(str5,",");
	test_assertStr(p,"abc");
	p = strtok(NULL," -,.");
	test_assertStr(p,"def");
	p = strtok(NULL," -,.");
	test_assertTrue(p == NULL);

	/* str6 */
	p = strtok(str6,",");
	test_assertStr(p,"abc");
	p = strtok(NULL," -,.");
	test_assertStr(p,"def");
	p = strtok(NULL," -,.");
	test_assertTrue(p == NULL);

	p = strtok(str7,"?");
	test_assertStr(p,"a");
	p = strtok(NULL,",");
	test_assertStr(p,"??b");
	p = strtok(NULL,"#,");
	test_assertStr(p,"c");
	p = strtok(NULL,"?");
	test_assertStr(p,NULL);

	test_caseSucceeded();
}

static void test_strcat() {
	char str1[7] = "abc";
	char str2[] = "def";
	test_caseStart("Testing strcat()");

	test_assertStr(strcat(str1,str2),"abcdef");
	test_assertStr(strcat(str1,""),"abcdef");

	test_caseSucceeded();
}

static void test_strncat() {
	char str1[10] = "abc";
	char str2[10] = "abc";
	char str3[10] = "abc";
	char str4[10] = "abc";
	char str5[10] = "abc";
	char str6[10] = "abc";
	char app[] = "def123";
	test_caseStart("Testing strnat()");

	test_assertStr(strncat(str1,app,3),"abcdef");
	test_assertStr(strncat(str2,app,1),"abcd");
	test_assertStr(strncat(str3,app,0),"abc");
	test_assertStr(strncat(str4,app,4),"abcdef1");
	test_assertStr(strncat(str5,app,5),"abcdef12");
	test_assertStr(strncat(str6,app,6),"abcdef123");

	test_caseSucceeded();
}

static void test_strcmp() {
	test_caseStart("Testing strcmp()");

	test_assertTrue(strcmp("","") == 0);
	test_assertTrue(strcmp("abc","abd") < 0);
	test_assertTrue(strcmp("abc","abb") > 0);
	test_assertTrue(strcmp("a","abc") < 0);
	test_assertTrue(strcmp("b","a") > 0);
	test_assertTrue(strcmp("1234","5678") < 0);
	test_assertTrue(strcmp("abc def","abc def") == 0);
	test_assertTrue(strcmp("123","123") == 0);

	test_caseSucceeded();
}

static void test_strncmp() {
	test_caseStart("Testing strncmp()");

	test_assertTrue(strncmp("","",0) == 0);
	test_assertTrue(strncmp("abc","abd",3) < 0);
	test_assertTrue(strncmp("abc","abb",3) > 0);
	test_assertTrue(strncmp("a","abc",1) == 0);
	test_assertTrue(strncmp("b","a",1) > 0);
	test_assertTrue(strncmp("1234","5678",4) < 0);
	test_assertTrue(strncmp("abc def","abc def",7) == 0);
	test_assertTrue(strncmp("123","123",3) == 0);
	test_assertTrue(strncmp("ab","ab",1) == 0);
	test_assertTrue(strncmp("abc","abd",2) == 0);
	test_assertTrue(strncmp("abc","abd",0) == 0);
	test_assertTrue(strncmp("xyz","abc",3) > 0);

	test_caseSucceeded();
}

static void test_strcasecmp() {
	test_caseStart("Testing strcasecmp()");

	test_assertTrue(strcasecmp("","") == 0);
	test_assertTrue(strcasecmp("aBc","abd") < 0);
	test_assertTrue(strcasecmp("abc","ABB") > 0);
	test_assertTrue(strcasecmp("A","Abc") < 0);
	test_assertTrue(strcasecmp("b","A") > 0);
	test_assertTrue(strcasecmp("1234","5678") < 0);
	test_assertTrue(strcasecmp("abc DEF","ABC def") == 0);
	test_assertTrue(strcasecmp("123","123") == 0);

	test_caseSucceeded();
}

static void test_strncasecmp() {
	test_caseStart("Testing strncasecmp()");

	test_assertTrue(strncasecmp("","abc",0) == 0);
	test_assertTrue(strncasecmp("","",0) == 0);
	test_assertTrue(strncasecmp("abc","ABd",3) < 0);
	test_assertTrue(strncasecmp("aBC","abb",3) > 0);
	test_assertTrue(strncasecmp("a","ABC",1) == 0);
	test_assertTrue(strncasecmp("b","a",1) > 0);
	test_assertTrue(strncasecmp("1234","5678",4) < 0);
	test_assertTrue(strncasecmp("abc DEF","ABC def",7) == 0);
	test_assertTrue(strncasecmp("123","123",3) == 0);
	test_assertTrue(strncasecmp("ab","aB",1) == 0);
	test_assertTrue(strncasecmp("aBc","Abd",2) == 0);
	test_assertTrue(strncasecmp("aBC","ABd",0) == 0);
	test_assertTrue(strncasecmp("xyz","aBc",3) > 0);

	test_caseSucceeded();
}

static void test_strchr() {
	char str1[] = "abcdef";
	test_caseStart("Testing strchr()");

	test_assertTrue(strchr(str1,'a') == str1);
	test_assertTrue(strchr(str1,'b') == str1 + 1);
	test_assertTrue(strchr(str1,'c') == str1 + 2);
	test_assertTrue(strchr(str1,'g') == NULL);

	test_caseSucceeded();
}

static void test_strchri() {
	char str1[] = "abcdef";
	test_caseStart("Testing strchri()");

	test_assertTrue(strchri(str1,'a') == 0);
	test_assertTrue(strchri(str1,'b') == 1);
	test_assertTrue(strchri(str1,'c') == 2);
	test_assertTrue(strchri(str1,'g') == (ssize_t)strlen(str1));

	test_caseSucceeded();
}

static void test_strrchr() {
	char str1[] = "abcdefabc";
	test_caseStart("Testing strrchr()");

	test_assertTrue(strrchr(str1,'a') == str1 + 6);
	test_assertTrue(strrchr(str1,'b') == str1 + 7);
	test_assertTrue(strrchr(str1,'c') == str1 + 8);
	test_assertTrue(strrchr(str1,'g') == NULL);
	test_assertTrue(strrchr(str1,'d') == str1 + 3);

	test_caseSucceeded();
}

static void test_strstr() {
	char str1[] = "abc def ghi";
	char str2[] = "";
	test_caseStart("Testing strstr()");

	test_assertTrue(strstr(str1,"abc") == str1);
	test_assertTrue(strstr(str1,"def") == str1 + 4);
	test_assertTrue(strstr(str1,"ghi") == str1 + 8);
	test_assertTrue(strstr(str1,"a") == str1);
	test_assertTrue(strstr(str1,"abc def ghi") == str1);
	test_assertTrue(strstr(str1,"abc ghi") == NULL);
	test_assertTrue(strstr(str1,"") == str1);
	test_assertTrue(strstr(str1,"def ghi") == str1 + 4);
	test_assertTrue(strstr(str1,"g") == str1 + 8);
	test_assertTrue(strstr(str2,"abc") == NULL);
	test_assertTrue(strstr(str2,"") == str2);

	test_caseSucceeded();
}

static void test_strcasestr() {
	char str1[] = "aBc DeF gHi";
	char str2[] = "";
	test_caseStart("Testing strcasestr()");

	test_assertTrue(strcasestr(str1,"Abc") == str1);
	test_assertTrue(strcasestr(str1,"deF") == str1 + 4);
	test_assertTrue(strcasestr(str1,"ghI") == str1 + 8);
	test_assertTrue(strcasestr(str1,"A") == str1);
	test_assertTrue(strcasestr(str1,"aBC DEf ghi") == str1);
	test_assertTrue(strcasestr(str1,"aBc ghi") == NULL);
	test_assertTrue(strcasestr(str1,"") == str1);
	test_assertTrue(strcasestr(str1,"dEF ghi") == str1 + 4);
	test_assertTrue(strcasestr(str1,"G") == str1 + 8);
	test_assertTrue(strcasestr(str2,"ABC") == NULL);
	test_assertTrue(strcasestr(str2,"") == str2);

	test_caseSucceeded();
}

static void test_strspn() {
	test_caseStart("Testing strspn()");

	test_assertSize(strspn("abc","def"),0);
	test_assertSize(strspn("abc","a"),1);
	test_assertSize(strspn("abc","ab"),2);
	test_assertSize(strspn("abc","abc"),3);
	test_assertSize(strspn("abc","bca"),3);
	test_assertSize(strspn("abc","cab"),3);
	test_assertSize(strspn("abc","cba"),3);
	test_assertSize(strspn("abc","bac"),3);
	test_assertSize(strspn("abc","b"),0);
	test_assertSize(strspn("abc","cdef"),0);
	test_assertSize(strspn("","123"),0);
	test_assertSize(strspn("",""),0);

	test_caseSucceeded();
}

static void test_strcspn() {
	test_caseStart("Testing strcspn()");

	test_assertSize(strcspn("abc","def"),3);
	test_assertSize(strcspn("abc","a"),0);
	test_assertSize(strcspn("abc","b"),1);
	test_assertSize(strcspn("abc","cdef"),2);
	test_assertSize(strcspn("","123"),0);
	test_assertSize(strcspn("",""),0);

	test_caseSucceeded();
}

static void test_strpbrk() {
	char str1[] = "test";
	char str2[] = "abc_def";
	char str3[] = "";
	test_caseStart("Testing strpbrk()");

	test_assertStr(strpbrk(str1,"test"),str1);
	test_assertStr(strpbrk(str1,"t"),str1);
	test_assertStr(strpbrk(str1,"e"),str1 + 1);
	test_assertStr(strpbrk(str1,"s"),str1 + 2);
	test_assertStr(strpbrk(str2,"fde"),str2 + 4);
	test_assertStr(strpbrk(str2,"def"),str2 + 4);
	test_assertStr(strpbrk(str2,"fed"),str2 + 4);
	test_assertStr(strpbrk(str2,"__"),str2 + 3);
	test_assertTrue(strpbrk(str2,"ghxyz") == NULL);
	test_assertTrue(strpbrk(str3,"a") == NULL);
	test_assertTrue(strpbrk(str3,"xyz") == NULL);
	test_assertTrue(strpbrk(str3,"") == NULL);
	test_assertTrue(strpbrk(str2,"") == NULL);
	test_assertTrue(strpbrk(str1,"") == NULL);

	test_caseSucceeded();
}

static void test_strcut() {
	char str1[] = "abc def ghi";
	char str2[] = "abc";
	char str3[] = "a";
	char str4[] = "";
	char str5[] = "123456";
	test_caseStart("Testing strcut()");

	test_assertStr(strcut(str1,4),"def ghi");
	test_assertStr(strcut(str2,3),"");
	test_assertStr(strcut(str3,0),"a");
	test_assertStr(strcut(str4,0),"");
	test_assertStr(strcut(str5,5),"6");

	test_caseSucceeded();
}

static void test_strlen() {
	test_caseStart("Testing strlen()");

	test_assertSize(strlen("abc"),3);
	test_assertSize(strlen(""),0);
	test_assertSize(strlen("abcdef"),6);
	test_assertSize(strlen("1"),1);

	test_caseSucceeded();
}

static void test_strnlen() {
	test_caseStart("Testing strnlen()");

	test_assertSSize(strnlen("abc",10),3);
	test_assertSSize(strnlen("",10),0);
	test_assertSSize(strnlen("abcdef",10),6);
	test_assertSSize(strnlen("1",10),1);
	test_assertSSize(strnlen("abc",3),3);
	test_assertSSize(strnlen("",0),0);
	test_assertSSize(strnlen("abcdef",6),6);
	test_assertSSize(strnlen("1",1),1);
	test_assertSSize(strnlen("abc",2),-1);
	test_assertSSize(strnlen("abcdef",5),-1);
	test_assertSSize(strnlen("1",0),-1);

	test_caseSucceeded();
}

static void test_tolower() {
	test_caseStart("Testing tolower()");

	test_assertInt(tolower('C'),'c');
	test_assertInt(tolower('A'),'a');
	test_assertInt(tolower('Z'),'z');
	test_assertInt(tolower('a'),'a');
	test_assertInt(tolower('x'),'x');
	test_assertInt(tolower('z'),'z');

	test_caseSucceeded();
}

static void test_toupper() {
	test_caseStart("Testing toupper()");

	test_assertInt(toupper('c'),'C');
	test_assertInt(toupper('a'),'A');
	test_assertInt(toupper('z'),'Z');
	test_assertInt(toupper('A'),'A');
	test_assertInt(toupper('X'),'X');
	test_assertInt(toupper('Z'),'Z');

	test_caseSucceeded();
}

static void test_isalnumstr() {
	test_caseStart("Testing isalnumstr()");

	test_assertTrue(isalnumstr("abc123"));
	test_assertTrue(isalnumstr("Axer123084"));
	test_assertTrue(isalnumstr(""));
	test_assertTrue(isalnumstr("a"));
	test_assertTrue(isalnumstr("0"));
	test_assertFalse(isalnumstr("_"));
	test_assertFalse(isalnumstr("abc123*"));
	test_assertFalse(isalnumstr("../*/\\"));
	test_assertFalse(isalnumstr("12.5"));

	test_caseSucceeded();
}

static void test_strmatch() {
	test_caseStart("Testing strmatch()");

	test_assertTrue(strmatch("abc*def","abcdef"));
	test_assertTrue(strmatch("abc*def","abcxdef"));
	test_assertTrue(strmatch("abc*def","abcTESTdef"));
	test_assertFalse(strmatch("abc*def","bcde"));
	test_assertFalse(strmatch("abc*def",""));
	test_assertFalse(strmatch("abc*def","abc"));
	test_assertFalse(strmatch("abc*def","abcde"));
	test_assertTrue(strmatch("abc*","abc"));
	test_assertTrue(strmatch("abc*","abcd"));
	test_assertTrue(strmatch("abc*","abcdef"));
	test_assertFalse(strmatch("abc*","bcdef"));
	test_assertFalse(strmatch("abc*","bc"));
	test_assertFalse(strmatch("abc*",""));
	test_assertTrue(strmatch("*def","def"));
	test_assertTrue(strmatch("*def","cdef"));
	test_assertTrue(strmatch("*def","abcdef"));
	test_assertFalse(strmatch("*def","abcde"));
	test_assertFalse(strmatch("*def","de"));
	test_assertFalse(strmatch("*def",""));
	test_assertTrue(strmatch("*",""));
	test_assertTrue(strmatch("*","a"));
	test_assertTrue(strmatch("*","abc"));
	test_assertTrue(strmatch("abc*x*def","abcxdef"));
	test_assertTrue(strmatch("abc*x*def","abcdddxbbbdef"));
	test_assertTrue(strmatch("abc*x*def","abcabcxdefdef"));
	test_assertFalse(strmatch("abc*x*def","abcdef"));
	test_assertFalse(strmatch("abc*x*def","abcdddef"));
	test_assertFalse(strmatch("abc*x*def",""));
	test_assertTrue(strmatch("abc*x*DEF*def","abcxDEFdef"));
	test_assertTrue(strmatch("abc*x*DEF*def","abcbbxccDEFdddef"));
	test_assertFalse(strmatch("abc*x*DEF*def","abcxDEdef"));
	test_assertFalse(strmatch("abc*x*DEF*def","abcbxcDEddef"));
	test_assertFalse(strmatch("abc*x*DEF*def","abcDEFdef"));
	test_assertFalse(strmatch("abc*x*DEF*def","abcDEFxdef"));
	test_assertFalse(strmatch("abc*x*DEF*def","abcbDEFcdef"));
	test_assertFalse(strmatch("abc*x*DEF*def",""));
	test_assertTrue(strmatch("abc","abc"));
	test_assertFalse(strmatch("abc","ab"));
	test_assertFalse(strmatch("abc","test"));
	test_assertFalse(strmatch("abc",""));

	test_caseSucceeded();
}

static void test_strtol() {
	struct StrtolTest {
		const char *str;
		uint base;
		long res;
	};
	StrtolTest tests[] = {
		{"1234",		10,	1234},
		{" \t12",		0,	12},
		{"+234",		10,	234},
		{"-1234",		10,	-1234},
		{"55",			8,	055},
		{"055",			0,	055},
		{"AbC",			16,	0xABC},
		{"0xaBC",		0,	0xABC},
		{sizeof(long) == 8 ? "0x8000000000000000" : "0x80000000",16,(long)(1UL << (sizeof(long) * 8 - 1))},
		{"aiz",			36,	13643},
		{"01101",		2,	13},
		{"0",			7,	0},
		{"-1",			7,	-1},
	};
	size_t i;
	long res;
	char *end;

	test_caseStart("Testing strtol()");

	for(i = 0; i < ARRAY_SIZE(tests); i++) {
		res = strtol(tests[i].str,&end,tests[i].base);
		test_assertLInt(res,tests[i].res);
		test_assertTrue(*end == '\0');
		res = strtol(tests[i].str,NULL,tests[i].base);
		test_assertLInt(res,tests[i].res);
	}

	test_caseSucceeded();
}
