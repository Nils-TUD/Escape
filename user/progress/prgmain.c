/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <bufio.h>
#include <proc.h>

#define CONS_WIDTH (80 - 3)

int main(void) {
	u32 p,i,j;
	printf("Waiting for fun...\n");
	for(p = 0; p <= 100; p++) {
		/* percent to console width */
		j = p == 0 ? 0 : CONS_WIDTH / (100. / p);

		printf("\r[");
		/* completed */
		for(i = 0; i < j; i++)
			printc('=');
		printc('>');
		/* uncompleted */
		for(i = j + 1; i <= CONS_WIDTH; i++)
			printc(' ');
		printc(']');

		/* wait */
		for(i = 0; i < 10000; i++)
			yield();
	}
	printf("Ready!\n");
	return 0;
}
