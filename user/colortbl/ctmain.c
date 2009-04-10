/**
 * @version		$Id: t2main.c 197 2009-04-08 18:01:51Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/proc.h>
#include <esc/fileio.h>
#include <stdlib.h>

int main(void) {
	u32 i,j;
	prints("    ");
	for(i = 0; i < 16; i++)
		printf("%02x ",i << 4);
	printc('\n');

	for(i = 0; i < 16; i++) {
		printf("%02x: ",i);
		for(j = 0; j < 16; j++) {
			char esc[] = {'\033','f',i,'\033','b',j,'\0'};
			printf("%s##\033r\x0 ",esc);
		}
		printc('\n');
	}
	flush();

	return EXIT_SUCCESS;
}
