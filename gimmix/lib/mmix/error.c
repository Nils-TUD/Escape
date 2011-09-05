/**
 * $Id: error.c 202 2011-05-10 10:49:04Z nasmussen $
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "common.h"
#include "mmix/io.h"
#include "mmix/error.h"

void error(const char *msg,...) {
	va_list ap;
	va_start(ap,msg);
	verror(msg,ap);
	// not reached
	va_end(ap);
}

void verror(const char *msg,va_list ap) {
	mvprintf(msg,ap);
	mprintf("\n");
	// ignore errors raised by the gcov stuff
	fclose(stderr);
	exit(EXIT_FAILURE);
}
