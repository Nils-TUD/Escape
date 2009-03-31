/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/fileio.h>
#include <stdlib.h>
#include <string.h>

#define USERNAME "hrniels"
#define PASSWORD "test"

int main(void) {
	char un[10];
	char pw[10];

	while(1) {
		printf("Username: ");
		scanl(un,10);
		printf("\033e\x0");
		printf("Password: ");
		scanl(pw,10);
		printf("\n\033e\x1");

		if(strcmp(un,USERNAME) != 0)
			printf("\033f\x4Sorry, invalid username. Try again!\033r\x0\n");
		else if(strcmp(pw,PASSWORD) != 0)
			printf("\033f\x4Sorry, invalid password. Try again!\033r\x0\n");
		else {
			printf("\033f\x2Login successfull.\033r\x0\n");
			break;
		}
	}

	return EXIT_SUCCESS;
}
