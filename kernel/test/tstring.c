/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include <video.h>
#include <string.h>

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
static void test_strcat(void);
static void test_strncat(void);
static void test_strcmp(void);
static void test_strncmp(void);
static void test_strchr(void);
static void test_strchri(void);
static void test_strrchr(void);
static void test_strstr(void);
static void test_strcspn(void);
static void test_strcut(void);
static void test_strlen(void);
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
	test_strcat();
	test_strncat();
	test_strcmp();
	test_strncmp();
	test_strchr();
	test_strchri();
	test_strrchr();
	test_strstr();
	test_strcspn();
	test_strcut();
	test_strlen();
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

static bool test_itoacpy(s32 n,cstring expected) {
	static s8 str[12];
	itoa(str,n);
	return test_assertStr(str,(string)expected);
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
	cstring s1 = "abc";
	cstring s2 = "def123456";
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
	s8 src[11] = "0123456789",dest[11];
	test_caseStart("Testing memcpy()");

	memcpy(dest,src,10);
	dest[10] = '\0';
	if(!test_assertStr(dest,(string)"0123456789")) return;
	memcpy(dest,src,4);
	dest[4] = '\0';
	if(!test_assertStr(dest,(string)"0123")) return;
	memcpy(dest,src,0);
	dest[0] = '\0';
	if(!test_assertStr(dest,(string)"")) return;

	test_caseSucceded();
}

static void test_memcmp(void) {
	const s8 *str1 = "abc", *str2 = "abcdef", *str3 = "def";
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
	s8 str1[10] = "abc";
	s8 str2[10] = "def";
	s8 str3[10] = "";
	s8 dest[4];
	test_caseStart("Testing memmove()");

	if(!test_assertStr(memmove(dest,str1,4),(string)"abc")) return;
	if(!test_assertStr(memmove(str1 + 1,str1,4),(string)"abc") || !test_assertStr(str1,(string)"aabc")) return;
	if(!test_assertStr(memmove(str2,str2 + 1,3),(string)"ef") || !test_assertStr(str2,(string)"ef")) return;
	if(!test_assertStr(memmove(str3,str3,0),(string)"")) return;
	if(!test_assertStr(memmove(str3,"abcdef",7),(string)"abcdef")) return;

	test_caseSucceded();
}

static void test_strcpy(void) {
	s8 target[20];
	test_caseStart("Testing strcpy()");

	if(!test_assertStr(strcpy(target,"abc"),(string)"abc")) return;
	if(!test_assertStr(strcpy(target,""),(string)"")) return;
	if(!test_assertStr(strcpy(target,"0123456789012345678"),(string)"0123456789012345678")) return;
	if(!test_assertStr(strcpy(target,"123"),(string)"123")) return;

	test_caseSucceded();
}

static void test_strncpy(void) {
	s8 target[20];
	s8 cmp1[] = {'a',0,0};
	s8 cmp2[] = {'d','e','f',0,0,0};
	test_caseStart("Testing strncpy()");

	if(!test_assertStr(strncpy(target,"abc",3),(string)"abc")) return;
	if(!test_assertStr(strncpy(target,"",0),(string)"")) return;
	if(!test_assertStr(strncpy(target,"0123456789012345678",19),(string)"0123456789012345678")) return;
	if(!test_assertStr(strncpy(target,"123",3),(string)"123")) return;
	if(!test_assertStr(strncpy(target,"abc",1),(string)"a")) return;
	if(!test_assertStr(strncpy(target,"abc",1),cmp1)) return;
	if(!test_assertStr(strncpy(target,"defghi",3),cmp2)) return;

	test_caseSucceded();
}

static void test_strcat(void) {
	s8 str1[7] = "abc";
	s8 str2[] = "def";
	test_caseStart("Testing strcat()");

	if(!test_assertStr(strcat(str1,str2),(string)"abcdef")) return;
	if(!test_assertStr(strcat(str1,""),(string)"abcdef")) return;

	test_caseSucceded();
}

static void test_strncat(void) {
	s8 str1[10] = "abc";
	s8 str2[10] = "abc";
	s8 str3[10] = "abc";
	s8 str4[10] = "abc";
	s8 str5[10] = "abc";
	s8 str6[10] = "abc";
	s8 app[] = "def123";
	test_caseStart("Testing strnat()");

	if(!test_assertStr(strncat(str1,app,3),(string)"abcdef")) return;
	if(!test_assertStr(strncat(str2,app,1),(string)"abcd")) return;
	if(!test_assertStr(strncat(str3,app,0),(string)"abc")) return;
	if(!test_assertStr(strncat(str4,app,4),(string)"abcdef1")) return;
	if(!test_assertStr(strncat(str5,app,5),(string)"abcdef12")) return;
	if(!test_assertStr(strncat(str6,app,6),(string)"abcdef123")) return;

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
	s8 str1[] = "abcdef";
	test_caseStart("Testing strchr()");

	if(!test_assertTrue(strchr(str1,'a') == str1)) return;
	if(!test_assertTrue(strchr(str1,'b') == str1 + 1)) return;
	if(!test_assertTrue(strchr(str1,'c') == str1 + 2)) return;
	if(!test_assertTrue(strchr(str1,'g') == NULL)) return;

	test_caseSucceded();
}

static void test_strchri(void) {
	s8 str1[] = "abcdef";
	test_caseStart("Testing strchri()");

	if(!test_assertTrue(strchri(str1,'a') == 0)) return;
	if(!test_assertTrue(strchri(str1,'b') == 1)) return;
	if(!test_assertTrue(strchri(str1,'c') == 2)) return;
	if(!test_assertTrue(strchri(str1,'g') == (s32)strlen(str1))) return;

	test_caseSucceded();
}

static void test_strrchr(void) {
	s8 str1[] = "abcdefabc";
	test_caseStart("Testing strrchr()");

	if(!test_assertTrue(strrchr(str1,'a') == str1 + 6)) return;
	if(!test_assertTrue(strrchr(str1,'b') == str1 + 7)) return;
	if(!test_assertTrue(strrchr(str1,'c') == str1 + 8)) return;
	if(!test_assertTrue(strrchr(str1,'g') == NULL)) return;
	if(!test_assertTrue(strrchr(str1,'d') == str1 + 3)) return;

	test_caseSucceded();
}

static void test_strstr(void) {
	s8 str1[] = "abc def ghi";
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

static void test_strcut(void) {
	s8 str1[] = "abc def ghi";
	s8 str2[] = "abc";
	s8 str3[] = "a";
	s8 str4[] = "";
	s8 str5[] = "123456";
	test_caseStart("Testing strcut()");

	if(!test_assertStr(strcut(str1,4),(string)"def ghi")) return;
	if(!test_assertStr(strcut(str2,3),(string)"")) return;
	if(!test_assertStr(strcut(str3,0),(string)"a")) return;
	if(!test_assertStr(strcut(str4,0),(string)"")) return;
	if(!test_assertStr(strcut(str5,5),(string)"6")) return;

	test_caseSucceded();
}

static void test_strlen(void) {
	test_caseStart("Testing strchr()");

	if(!test_assertUInt(strlen((string)"abc"),3)) return;
	if(!test_assertUInt(strlen((string)""),0)) return;
	if(!test_assertUInt(strlen((string)"abcdef"),6)) return;
	if(!test_assertUInt(strlen((string)"1"),1)) return;

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
