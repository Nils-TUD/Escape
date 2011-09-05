/**
 * $Id: abstime.c 92 2010-12-19 16:36:54Z nasmussen $
 */

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

int main(void) {
	printf("#define ABSTIME %ld\n",time(NULL));
	return EXIT_SUCCESS;
}
