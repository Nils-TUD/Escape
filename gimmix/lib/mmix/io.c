/**
 * $Id: io.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "common.h"
#include "mmix/io.h"
#include "mmix/error.h"

#define MAX_FMT_LEN		1024

static const char *fmt_translate(const char *fmt);
static char fmt_getLength(char l);

octa mstrtoo(const char *str,char **endptr,int base) {
	if(sizeof(long) == sizeof(octa))
		return (octa)strtoul(str,endptr,base);
	if(sizeof(long long) == sizeof(octa))
		return (octa)strtoull(str,endptr,base);
	error("Unsupported platform");
	return 0;
}

int mscanf(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	int res = vscanf(fmt_translate(fmt),ap);
	va_end(ap);
	return res;
}

int msscanf(const char *str,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	int res = vsscanf(str,fmt_translate(fmt),ap);
	va_end(ap);
	return res;
}

int mfscanf(FILE *f,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	int res = vfscanf(f,fmt_translate(fmt),ap);
	va_end(ap);
	return res;
}

int mprintf(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	int res = mvprintf(fmt,ap);
	va_end(ap);
	return res;
}

int mvprintf(const char *fmt,va_list ap) {
	return vprintf(fmt_translate(fmt),ap);
}

int msnprintf(char *str,size_t size,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	int res = mvsnprintf(str,size,fmt,ap);
	va_end(ap);
	return res;
}

int mvsnprintf(char *str,size_t size,const char *fmt,va_list ap) {
	return vsnprintf(str,size,fmt_translate(fmt),ap);
}

int mfprintf(FILE *f,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	int res = vfprintf(f,fmt_translate(fmt),ap);
	va_end(ap);
	return res;
}

int mvfprintf(FILE *f,const char *fmt,va_list ap) {
	return vfprintf(f,fmt_translate(fmt),ap);
}

static const char *fmt_translate(const char *fmt) {
	static char prffmt[MAX_FMT_LEN + 1];
	bool infmt = false;
	int j = 0;
	for(int i = 0; j < MAX_FMT_LEN; i++) {
		if(!infmt) {
			if(fmt[i] == '%')
				infmt = true;
		}
		else {
			switch(fmt[i]) {
				case 'd':
				case 'i':
				case 'o':
				case 'u':
				case 'x':
				case 'X': {
					char l = fmt_getLength(fmt[i - 1]);
					if(l)
						prffmt[j - 1] = l;
					else {
						prffmt[j - 1] = prffmt[j];
						j--;
					}
					infmt = false;
				}
				break;

				case 'c':
				case 'e':
				case 'E':
				case 'f':
				case 'g':
				case 'G':
				case 's':
				case 'p':
				case 'n':
				case '%':
					infmt = false;
					break;
			}
		}
		prffmt[j++] = fmt[i];
	}
	prffmt[j] = '\0';
	return prffmt;
}

static char fmt_getLength(char l) {
	switch(l) {
		case 'B':
		case 'W':
			return 'h';
		case 'T':
			if(sizeof(long) == 4)
				return 'l';
			return '\0';
		case 'O':
			if(sizeof(long) == 8)
				return 'l';
			if(sizeof(long long) == 8)
				return 'L';
			error("Unsupported platform");
	}
	return l;
}
