/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifdef IN_KERNEL
#	include "../../kernel/h/common.h"
#	include "../../kernel/h/util.h"
#	include "../../kernel/h/paging.h"
#else
#	include "../../libc/h/common.h"
/* for exit and debugf (ASSERT) */
#	include "../../libc/h/proc.h"
#	include "../../libc/h/debug.h"
#endif

#include "../h/string.h"

s32 atoi(cstring str) {
	s32 i = 0;
	bool neg = false;
	s8 c;

	ASSERT(str != NULL,"str == NULL");

	/* skip leading whitespace */
	while(isspace(*str))
		str++;
	/* negative? */
	if(*str == '-') {
		neg = true;
		str++;
	}
	/* read number */
	while((c = *str) >= '0' && c <= '9') {
		i = i * 10 + c - '0';
		str++;
	}
	/* switch sign? */
	if(neg)
		i = -i;
	return i;
}

void itoa(string target,s32 n) {
	s8 *s = target,*a = target,*b;

	ASSERT(target != NULL,"target == NULL");

	/* handle sign */
	if(n < 0) {
		*s++ = '-';
		a = s;
	}
	/* 0 is a special case */
	else if(n == 0)
		*s++ = '0';
	/* use negative numbers because otherwise we would get problems with -2147483648 */
	else
		n = -n;

	/* divide by 10 and put the remainer in the string */
	while(n < 0) {
		*s++ = -(n % 10) + '0';
		n = n / 10;
	}
	/* terminate */
	*s = '\0';

	/* reverse string */
	b = s - 1;
	while(a < b) {
		s8 t = *a;
		*a++ = *b;
		*b-- = t;
	}
}

void *memchr(const void *buffer,s32 c,u32 count) {
	cstring str = buffer;
	while(*str && count-- > 0) {
		if(*str == c)
			return (void*)str;
		str++;
	}
	return NULL;
}

void *memcpy(void *dest,const void *src,u32 len) {
	u8 *d = dest;
	const u8 *s = src;
	while(len--) {
#if IN_KERNEL
		if(d == 0xc0965e51 || s == 0xc0965e51) {
			panic("HERE IT IS\n");
		}
#endif
		*d++ = *s++;
	}
	return dest;
}

s32 memcmp(const void *str1,const void *str2,u32 count) {
	const u8 *s1 = str1;
	const u8 *s2 = str2;
	while(count-- > 0) {
		if(*s1++ != *s2++)
			return s1[-1] < s2[-1] ? -1 : 1;
	}
	return 0;
}

void memset(void *addr,u32 value,u32 count) {
	u32 rCount = count % sizeof(u32);
	u32 dwordCount = (count - rCount) / sizeof(u32);
	u32 *dptr = (u32*)addr;
	u8 *bptr;
	while(dwordCount-- > 0) {
		*dptr++ = value;
	}
	bptr = (u8*)dptr;
	while(rCount-- > 0) {
		*bptr++ = value;
	}
}

string strcpy(string to,cstring from) {
	string res = to;

	ASSERT(from != NULL,"from == NULL");
	ASSERT(to != NULL,"to == NULL");

	while(*from) {
		*to++ = *from++;
	}
	*to = '\0';
	return res;
}

string strncpy(string to,cstring from,u32 count) {
	string res = to;

	ASSERT(from != NULL,"from == NULL");
	ASSERT(to != NULL,"to == NULL");

	/* copy source string */
	while(*from && count > 0) {
		*to++ = *from++;
		count--;
	}
	/* terminate & fill with \0 if needed */
	do {
		*to++ = '\0';
	}
	while(count-- > 0);
	return res;
}

string strcat(string str1,cstring str2) {
	string res = str1;

	ASSERT(str1 != NULL,"str1 == NULL");
	ASSERT(str2 != NULL,"str2 == NULL");

	/* walk to end */
	while(*str1)
		str1++;
	/* append */
	while(*str2)
		*str1++ = *str2++;
	return res;
}

string strncat(string str1,cstring str2,u32 count) {
	string res = str1;

	ASSERT(str1 != NULL,"str1 == NULL");
	ASSERT(str2 != NULL,"str2 == NULL");

	/* walk to end */
	while(*str1)
		str1++;
	/* append */
	while(*str2 && count-- > 0)
		*str1++ = *str2++;
	/* terminate */
	*str1 = '\0';
	return res;
}

s32 strcmp(cstring str1,cstring str2) {
	s8 c1 = *str1,c2 = *str2;

	ASSERT(str1 != NULL,"str1 == NULL");
	ASSERT(str2 != NULL,"str2 == NULL");

	while(c1 && c2) {
		/* different? */
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		c1 = *++str1;
		c2 = *++str2;
	}
	/* check which strings are finished */
	if(!c1 && !c2)
		return 0;
	if(!c1 && c2)
		return -1;
	return 1;
}

s32 strncmp(cstring str1,cstring str2,u32 count) {
	ASSERT(str1 != NULL,"str1 == NULL");
	ASSERT(str2 != NULL,"str2 == NULL");

	while(count-- > 0) {
		if(*str1++ != *str2++)
			return str1[-1] < str2[-1] ? -1 : 1;
	}
	return 0;
}

string strchr(cstring str,s32 ch) {
	ASSERT(str != NULL,"str == NULL");

	while(*str) {
		if(*str++ == ch)
			return (string)(str - 1);
	}
	return NULL;
}

s32 strchri(cstring str,s32 ch) {
	cstring save = str;

	ASSERT(str != NULL,"str == NULL");

	while(*str) {
		if(*str++ == ch)
			return str - save - 1;
	}
	return str - save;
}

string strrchr(cstring str,s32 ch) {
	string pos = NULL;

	ASSERT(str != NULL,"str == NULL");

	while(*str) {
		if(*str++ == ch)
			pos = (string)(str - 1);
	}
	return pos;
}

string strstr(cstring str1,cstring str2) {
	string res = NULL;
	string sub;

	ASSERT(str1 != NULL,"str1 == NULL");
	ASSERT(str2 != NULL,"str2 == NULL");

	/* handle special case to prevent looping the string */
	if(!*str2)
		return NULL;
	while(*str1) {
		/* matching char? */
		if(*str1++ == *str2) {
			res = (string)--str1;
			sub = (string)str2;
			/* continue until the strings don't match anymore */
			while(*sub && *str1 == *sub) {
				str1++;
				sub++;
			}
			/* complete substring matched? */
			if(!*sub)
				return res;
		}
	}
	return NULL;
}

u32 strcspn(cstring str1,cstring str2) {
	u32 count = 0;

	ASSERT(str1 != NULL,"str1 == NULL");
	ASSERT(str2 != NULL,"str2 == NULL");

	while(*str1) {
		if(strchr(str2,*str1++) != NULL)
			return count;
		count++;
	}
	return count;
}

string strcut(string str,u32 count) {
	string res = str;

	ASSERT(str != NULL,"str == NULL");

	if(count > 0) {
		str += count;
		while(*str) {
			*(str - count) = *str;
			str++;
		}
		*(str - count) = '\0';
	}
	return res;
}

u32 strlen(cstring str) {
	u32 len = 0;

	ASSERT(str != NULL,"str == NULL");

	while(*str++)
		len++;
	return len;
}

s32 tolower(s32 ch) {
	if(ch >= 'A' && ch <= 'Z')
		return ch - ('A' - 'a');
	return ch;
}

s32 toupper(s32 ch) {
	if(ch >= 'a' && ch <= 'z')
		return ch + ('A' - 'a');
	return ch;
}

bool isalnumstr(cstring str) {
	while(*str) {
		if(!isalnum(*str++))
			return false;
	}
	return true;
}

bool isalnum(s32 c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

bool isalpha(s32 c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isdigit(s32 c) {
	return (c >= '0' && c <= '9');
}

bool islower(s32 c) {
	return (c >= 'a' && c <= 'z');
}

bool isspace(s32 c) {
	return (c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f' || c == '\v');
}

bool isupper(s32 c) {
	return (c >= 'A' && c <= 'Z');
}
