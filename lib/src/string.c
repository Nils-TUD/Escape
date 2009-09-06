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

#include <types.h>
#include <errors.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#ifdef IN_KERNEL
#	include <util.h>
#	include <paging.h>
#else
/* for exit and debugf (vassert) */
#	include <esc/proc.h>
#	include <esc/fileio.h>
#endif

#define MAX_ERR_LEN		255

s32 atoi(const char *str) {
	s32 i = 0;
	bool neg = false;
	char c;

	vassert(str != NULL,"str == NULL");

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

s64 atol(const char *str) {
	s64 i = 0;
	bool neg = false;
	char c;

	vassert(str != NULL,"str == NULL");

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

void itoa(char *target,s32 n) {
	char *s = target,*a = target,*b;

	vassert(target != NULL,"target == NULL");

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
		char t = *a;
		*a++ = *b;
		*b-- = t;
	}
}

void *memchr(const void *buffer,s32 c,u32 count) {
	const char *str = (const char*)buffer;
	while(*str && count-- > 0) {
		if(*str == c)
			return (void*)str;
		str++;
	}
	return NULL;
}

void *memcpy(void *dest,const void *src,u32 len) {
	/* copy dwords first with loop-unrolling */
	u32 *ddest = (u32*)dest;
	u32 *dsrc = (u32*)src;
	while(len >= sizeof(u32) * 8) {
		*ddest = *dsrc;
		*(ddest + 1) = *(dsrc + 1);
		*(ddest + 2) = *(dsrc + 2);
		*(ddest + 3) = *(dsrc + 3);
		*(ddest + 4) = *(dsrc + 4);
		*(ddest + 5) = *(dsrc + 5);
		*(ddest + 6) = *(dsrc + 6);
		*(ddest + 7) = *(dsrc + 7);
		ddest += 8;
		dsrc += 8;
		len -= sizeof(u32) * 8;
	}
	/* copy remaining dwords */
	while(len >= sizeof(u32)) {
		*ddest++ = *dsrc++;
		len -= sizeof(u32);
	}

	/* copy remaining bytes */
	u8 *bdest = (u8*)ddest;
	u8 *bsrc = (u8*)dsrc;
	while(len-- > 0)
		*bdest++ = *bsrc++;
	return dest;
}

s32 memcmp(const void *str1,const void *str2,u32 count) {
	const u8 *s1 = (const u8*)str1;
	const u8 *s2 = (const u8*)str2;
	while(count-- > 0) {
		if(*s1++ != *s2++)
			return s1[-1] < s2[-1] ? -1 : 1;
	}
	return 0;
}

void memclear(void *addr,u32 count) {
	/* clear dwords first with loop-unrolling */
	u32 *daddr = (u32*)addr;
	while(count >= sizeof(u32) * 8) {
		*daddr = 0;
		*(daddr + 1) = 0;
		*(daddr + 2) = 0;
		*(daddr + 3) = 0;
		*(daddr + 4) = 0;
		*(daddr + 5) = 0;
		*(daddr + 6) = 0;
		*(daddr + 7) = 0;
		daddr += 8;
		count -= sizeof(u32) * 8;
	}
	/* copy remaining dwords */
	while(count >= sizeof(u32)) {
		*daddr++ = 0;
		count -= sizeof(u32);
	}

	/* clear remaining bytes */
	u8 *baddr = (u8*)daddr;
	while(count-- > 0)
		*baddr++ = 0;
}

void memset(void *addr,u8 value,u32 count) {
	u32 dwval = (value << 24) | (value << 16) | (value << 8) | value;
	u32 *dwaddr = (u32*)addr;
	while(count >= sizeof(u32)) {
		*dwaddr++ = dwval;
		count -= sizeof(u32);
	}

	u8 *baddr = (u8*)dwaddr;
	while(count-- > 0)
		*baddr++ = value;
}

void *memmove(void *dest,const void *src,u32 count) {
	u8 *s,*d;
	/* nothing to do? */
	if((u8*)dest == (u8*)src)
		return dest;

	/* moving forward */
	if((u8*)dest > (u8*)src) {
		u32 *dsrc = (u32*)((u8*)src + count - sizeof(u32));
		u32 *ddest = (u32*)((u8*)dest + count - sizeof(u32));
		while(count >= sizeof(u32)) {
			*ddest-- = *dsrc--;
			count -= sizeof(u32);
		}
		s = (u8*)dsrc + (sizeof(u32) - 1);
		d = (u8*)ddest + (sizeof(u32) - 1);
		while(count-- > 0)
			*d-- = *s--;
	}
	/* moving backwards */
	else
		memcpy(dest,src,count);

	return dest;
}

char *strcpy(char *to,const char *from) {
	char *res = to;

	vassert(from != NULL,"from == NULL");
	vassert(to != NULL,"to == NULL");

	while(*from) {
		*to++ = *from++;
	}
	*to = '\0';
	return res;
}

char *strncpy(char *to,const char *from,u32 count) {
	char *res = to;

	vassert(from != NULL,"from == NULL");
	vassert(to != NULL,"to == NULL");

	/* copy source string */
	while(*from && count > 0) {
		*to++ = *from++;
		count--;
	}
	/* terminate & fill with \0 if needed */
	while(count-- > 0)
		*to++ = '\0';
	return res;
}

char *strcat(char *str1,const char *str2) {
	char *res = str1;

	vassert(str1 != NULL,"str1 == NULL");
	vassert(str2 != NULL,"str2 == NULL");

	/* walk to end */
	while(*str1)
		str1++;
	/* append */
	while(*str2)
		*str1++ = *str2++;
	/* terminate */
	*str1 = '\0';
	return res;
}

char *strncat(char *str1,const char *str2,u32 count) {
	char *res = str1;

	vassert(str1 != NULL,"str1 == NULL");
	vassert(str2 != NULL,"str2 == NULL");

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

s32 strcmp(const char *str1,const char *str2) {
	char c1 = *str1,c2 = *str2;

	vassert(str1 != NULL,"str1 == NULL");
	vassert(str2 != NULL,"str2 == NULL");

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

s32 strncmp(const char *str1,const char *str2,u32 count) {
	s32 rem = count;
	vassert(str1 != NULL,"str1 == NULL");
	vassert(str2 != NULL,"str2 == NULL");

	while(*str1 && *str2 && rem-- > 0) {
		if(*str1++ != *str2++)
			return str1[-1] < str2[-1] ? -1 : 1;
	}
	if(rem <= 0)
		return 0;
	if(*str1 && !*str2)
		return 1;
	return -1;
}

char *strchr(const char *str,s32 ch) {
	vassert(str != NULL,"str == NULL");

	while(*str) {
		if(*str++ == ch)
			return (char*)(str - 1);
	}
	return NULL;
}

s32 strchri(const char *str,s32 ch) {
	const char *save = str;

	vassert(str != NULL,"str == NULL");

	while(*str) {
		if(*str++ == ch)
			return str - save - 1;
	}
	return str - save;
}

char *strrchr(const char *str,s32 ch) {
	char *pos = NULL;

	vassert(str != NULL,"str == NULL");

	while(*str) {
		if(*str++ == ch)
			pos = (char*)(str - 1);
	}
	return pos;
}

char *strstr(const char *str1,const char *str2) {
	char *res = NULL;
	char *sub;

	vassert(str1 != NULL,"str1 == NULL");
	vassert(str2 != NULL,"str2 == NULL");

	/* handle special case to prevent looping the string */
	if(!*str2)
		return NULL;
	while(*str1) {
		/* matching char? */
		if(*str1++ == *str2) {
			res = (char*)--str1;
			sub = (char*)str2;
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

u32 strspn(const char *str1,const char *str2) {
	u32 count = 0;

	vassert(str1 != NULL,"str1 == NULL");
	vassert(str2 != NULL,"str2 == NULL");

	while(*str1) {
		if(strchr(str2,*str1++) == NULL)
			return count;
		count++;
	}
	return count;
}

u32 strcspn(const char *str1,const char *str2) {
	u32 count = 0;

	vassert(str1 != NULL,"str1 == NULL");
	vassert(str2 != NULL,"str2 == NULL");

	while(*str1) {
		if(strchr(str2,*str1++) != NULL)
			return count;
		count++;
	}
	return count;
}

char *strpbrk(const char *str1,const char *str2) {
	char *s2;

	vassert(str1 != NULL,"str1 == NULL");
	vassert(str2 != NULL,"str2 == NULL");

	while(*str1) {
		s2 = (char*)str2;
		while(*s2) {
			if(*s2 == *str1)
				return (char*)str1;
			s2++;
		}
		str1++;
	}
	return NULL;
}

char *strtok(char *str,const char *delimiters) {
	static char *last = NULL;
	char *res;
	u32 start,end;
	if(str)
		last = str;
	if(last == NULL)
		return NULL;

	/* find first char of the token */
	start = strspn(last,delimiters);
	last += start;
	/* reached end? */
	if(*last == '\0') {
		last = NULL;
		return NULL;
	}
	/* store beginning */
	res = last;

	/* find end of the token */
	end = strcspn(last,delimiters);
	/* have we reached the end? */
	if(last[end] == '\0')
		last = NULL;
	else {
		last[end] = '\0';
		/* we want to continue at that place next time */
		last += end + 1;
	}

	return res;
}

char *strcut(char *str,u32 count) {
	char *res = str;

	vassert(str != NULL,"str == NULL");

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

u32 strlen(const char *str) {
	u32 len = 0;

	vassert(str != NULL,"str == NULL");

	while(*str++)
		len++;
	return len;
}

s32 strnlen(const char *str,s32 max) {
	s32 len = 0;
	vassert(str != NULL,"str == NULL");
	vassert(max >= 0,"max < 0");

	while(*str && len <= max) {
		str++;
		len++;
	}
	if(len > max)
		return -1;
	return len;
}

bool isalnumstr(const char *str) {
	while(*str) {
		if(!isalnum(*str++))
			return false;
	}
	return true;
}

char *strerror(s32 errnum) {
	static char msg[MAX_ERR_LEN + 1];
	switch(errnum) {
		case ERR_FILE_IN_USE:
			strcpy(msg,"The file is already in use");
			break;

		case ERR_INVALID_SYSC_ARGS:
			strcpy(msg,"Invalid syscall-arguments");
			break;

		case ERR_MAX_PROC_FDS:
			strcpy(msg,"You have reached the max. possible file-descriptors");
			break;

		case ERR_NO_FREE_FD:
			strcpy(msg,"The max. global, open files have been reached");
			break;

		case ERR_VFS_NODE_NOT_FOUND:
			strcpy(msg,"Unable to resolve the path");
			break;

		case ERR_INVALID_FD:
			strcpy(msg,"Invalid file-descriptor");
			break;

		case ERR_INVALID_FILE:
			strcpy(msg,"Invalid file");
			break;

		case ERR_NO_READ_PERM:
			strcpy(msg,"No read-permission");
			break;

		case ERR_NO_WRITE_PERM:
			strcpy(msg,"No write-permission");
			break;

		case ERR_INV_SERVICE_NAME:
			strcpy(msg,"Invalid service name. Alphanumeric, not empty name expected!");
			break;

		case ERR_NOT_ENOUGH_MEM:
			strcpy(msg,"Not enough memory!");
			break;

		case ERR_SERVICE_EXISTS:
			strcpy(msg,"The service with desired name does already exist!");
			break;

		case ERR_PROC_DUP_SERVICE:
			strcpy(msg,"You are already a service!");
			break;

		case ERR_PROC_DUP_SERVICE_USE:
			strcpy(msg,"You are already using the requested service!");
			break;

		case ERR_SERVICE_NOT_IN_USE:
			strcpy(msg,"You are not using the service at the moment!");
			break;

		case ERR_NOT_OWN_SERVICE:
			strcpy(msg,"The service-node is not your own!");
			break;

		case ERR_IO_MAP_RANGE_RESERVED:
			strcpy(msg,"The given io-port range is reserved!");
			break;

		case ERR_IOMAP_NOT_PRESENT:
			strcpy(msg,"The io-port-map is not present (have you reserved ports?)");
			break;

		case ERR_INTRPT_LISTENER_MSGLEN:
			strcpy(msg,"The length of the interrupt-notify-message is too long!");
			break;

		case ERR_INVALID_IRQ_NUMBER:
			strcpy(msg,"The given IRQ-number is invalid!");
			break;

		case ERR_IRQ_LISTENER_MISSING:
			strcpy(msg,"The IRQ-listener is not present!");
			break;

		case ERR_NO_CLIENT_WAITING:
			strcpy(msg,"No client is currently waiting");
			break;

		case ERR_FS_NOT_FOUND:
			strcpy(msg,"Filesystem-service not found");
			break;

		case ERR_INVALID_SIGNAL:
			strcpy(msg,"Invalid signal");
			break;

		case ERR_INVALID_PID:
			strcpy(msg,"Invalid process-id");
			break;

		case ERR_NO_DIRECTORY:
			strcpy(msg,"A part of the path is no directory");
			break;

		case ERR_PATH_NOT_FOUND:
			strcpy(msg,"Path not found");
			break;

		case ERR_FS_READ_FAILED:
			strcpy(msg,"Read from fs failed");
			break;

		case ERR_INVALID_PATH:
			strcpy(msg,"Invalid path");
			break;

		case ERR_INVALID_NODENO:
			strcpy(msg,"Invalid Node-Number");
			break;

		case ERR_SERVUSE_SEEK:
			strcpy(msg,"seek() is not possible for service-usages!");
			break;

		case ERR_MAX_PROCS_REACHED:
			strcpy(msg,"No free process-slots!");
			break;

		case ERR_NODE_EXISTS:
			strcpy(msg,"Node does already exist!");
			break;

		case ERR_INVALID_ELF_BINARY:
			strcpy(msg,"The ELF-binary is invalid!");
			break;

		case ERR_SHARED_MEM_EXISTS:
			strcpy(msg,"The shared memory with specified name or address-range does already exist!");
			break;

		case ERR_SHARED_MEM_NAME:
			strcpy(msg,"The shared memory name is invalid!");
			break;

		case ERR_SHARED_MEM_INVALID:
			strcpy(msg,"The shared memory is invalid!");
			break;

		case ERR_LOCK_NOT_FOUND:
			strcpy(msg,"Lock not found!");
			break;

		case ERR_INVALID_TID:
			strcpy(msg,"Invalid thread-id");
			break;

		case ERR_UNSUPPORTED_OPERATION:
			strcpy(msg,"Unsupported operation");
			break;

		case ERR_FS_INODE_ALLOC:
			strcpy(msg,"Inode-allocation failed");
			break;

		case ERR_FS_WRITE_FAILED:
			strcpy(msg,"Write-operation failed");
			break;

		case ERR_FS_INODE_NOT_FOUND:
			strcpy(msg,"Inode not found");
			break;

		case ERR_FS_IS_DIRECTORY:
			strcpy(msg,"Its a directory");
			break;

		case ERR_FS_FILE_EXISTS:
			strcpy(msg,"The file exists");
			break;

		case ERR_FS_DIR_NOT_EMPTY:
			strcpy(msg,"Directory is not empty");
			break;

		case ERR_FS_INVALID_DRV_NAME:
			strcpy(msg,"Invalid driver-name");
			break;

		case ERR_FS_INVALID_FS_NAME:
			strcpy(msg,"Invalid filesystem-name");
			break;

		case ERR_FS_MNT_POINT_EXISTS:
			strcpy(msg,"Mount-point exists");
			break;

		case ERR_FS_DEVICE_NOT_FOUND:
			strcpy(msg,"Device not found");
			break;

		case ERR_FS_NO_MNT_POINT:
			strcpy(msg,"No mount-point");
			break;

		case ERR_FS_INIT_FAILED:
			strcpy(msg,"Initialisation of filesystem failed");
			break;

		case ERR_FS_LINK_DEVICE:
			strcpy(msg,"Hardlink to a different device not possible");
			break;

		case ERR_MOUNT_VIRT_PATH:
			strcpy(msg,"Mount in virtual directories not supported");
			break;

		case ERR_NO_FILE_OR_LINK:
			strcpy(msg,"No file or link");
			break;

		default:
			strcpy(msg,"No error");
			break;
	}
	return msg;
}
