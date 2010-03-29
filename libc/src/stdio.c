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
#include <esc/io.h>
#include <esc/fileio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

int putchar(int c) {
	return printc(c);
}

int putc(int c,FILE *file) {
	return fprintc(file,c);
}

int puts(const char *str) {
	return prints(str);
}

int fputc(int c,FILE *file) {
	return fprintc(file,c);
}

int fputs(const char *str,FILE *file) {
	return fprints(file,str);
}

int getchar(void) {
	return scanc();
}

int getc(FILE *file) {
	return fscanc(file);
}

int fgetc(FILE *file) {
	return fscanc(file);
}

int ungetc(int c,FILE *file) {
	return fscanback(file,c);
}

int gets(char *buffer) {
	/* TODO */
	return scans(buffer,2 << 31);
}

char *fgets(char *str,int max,FILE *file) {
	/* TODO error-handling correct? */
	if(fscanl(file,str,max) == 0)
		return NULL;
	return str;
}

int isatty(int fd) {
	return isterm(fd);
}

int sprintf(char *str,const char *fmt,...) {
	va_list ap;
	int res;
	va_start(ap,fmt);
	res = vsnprintf(str,-1,fmt,ap);
	va_end(ap);
	return res;
}

int vsprintf(char *str,const char *fmt,va_list ap) {
	return vsnprintf(str,-1,fmt,ap);
}

void perror(const char *msg) {
	printe(msg);
}
