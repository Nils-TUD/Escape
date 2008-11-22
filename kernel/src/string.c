/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/string.h"

s32 atoi(cstring str) {
	s32 i = 0;
	bool neg = false;
	s8 c;
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

void *memchr(const void *buffer,s32 c,size_t count) {
	cstring str = buffer;
	while(*str && count-- > 0) {
		if(*str == c)
			return (void*)str;
		str++;
	}
	return NULL;
}

void *memcpy(void *dest,const void *src,size_t len) {
	u8 *d = dest;
	const u8 *s = src;
	while(len--) {
		*d++ = *s++;
	}
	return dest;
}

s32 memcmp(const void *str1,const void *str2,size_t count) {
	const u8 *s1 = str1;
	const u8 *s2 = str2;
	while(count-- > 0) {
		if(*s1++ != *s2++)
			return s1[-1] < s2[-1] ? -1 : 1;
	}
	return 0;
}

void memset(void *addr,u32 value,size_t count) {
	u8 *ptr = (u8*)addr;
	while(count-- > 0) {
		*ptr++ = value;
	}
}

string strcpy(string to,cstring from) {
	string res = to;
	while(*from) {
		*to++ = *from++;
	}
	*to = '\0';
	return res;
}

string strncpy(string to,cstring from,size_t count) {
	string res = to;
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
	/* walk to end */
	while(*str1)
		str1++;
	/* append */
	while(*str2)
		*str1++ = *str2++;
	return res;
}

string strncat(string str1,cstring str2,size_t count) {
	string res = str1;
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

s32 strncmp(cstring str1,cstring str2,size_t count) {
	while(count-- > 0) {
		if(*str1++ != *str2++)
			return str1[-1] < str2[-1] ? -1 : 1;
	}
	return 0;
}

string strchr(cstring str,s32 ch) {
	while(*str) {
		if(*str++ == ch)
			return (string)(str - 1);
	}
	return NULL;
}

s32 strchri(cstring str,s32 ch) {
	s32 pos = 0;
	while(*str) {
		if(*str++ == ch)
			return pos;
		pos++;
	}
	return pos;
}

string strrchr(cstring str,s32 ch) {
	string pos = NULL;
	while(*str) {
		if(*str++ == ch)
			pos = (string)(str - 1);
	}
	return pos;
}

string strstr(cstring str1,cstring str2) {
	string res = NULL;
	string sub;
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

size_t strcspn(cstring str1,cstring str2) {
	size_t count = 0;
	while(*str1) {
		if(strchr(str2,*str1++) != NULL)
			return count;
		count++;
	}
	return count;
}

string strcut(string str,u32 count) {
	string res = str;
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

size_t strlen(string str) {
	size_t len = 0;
	while(*str++)
		len++;
	return len;
}

s32 tolower(s32 ch) {
	if(isupper(ch))
		return ch - ('A' - 'a');
	return ch;
}

s32 toupper(s32 ch) {
	if(islower(ch))
		return ch + ('A' - 'a');
	return ch;
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
