/**
 * @version		$Id: main.c 60 2008-11-16 18:29:11Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>
#include <proc.h>

s32 main(void) {
	/*u32 i;
	debugf("Hi, my pid is %d, parent %d\n",getpid(),getppid());
	for(i = 0;i < 10;i++) {
		debugf("Idling a bit...\n");
	}
	if(!fork()) {
		debugf("HAHA not dead yet :P\n");
		while(1) {
			debugf("Process %d\n",getpid());
		}
	}
	for(i = 0;i < 3;i++) {
		fork();
	}
	debugf("Bye (%d)\n",getpid());*/
	return 123;
}
