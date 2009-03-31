/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/fileio.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

int putchar(int c) {
	return printc(c);
}

int putc(FILE *file,int c) {
	return fprintc(file,c);
}

int puts(const char *str) {
	return prints(str);
}

int fputs(FILE *file,const char *str) {
	return fprints(file,str);
}

int getchar(void) {
	return scanc();
}

int getc(FILE *file) {
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

void perror(const char *msg) {
	fprintf(stderr,"%s: %s\n",msg,strerror(errno));
}
