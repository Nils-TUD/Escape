/**
 * $Id: tmain.c 202 2011-05-10 10:49:04Z nasmussen $
 */

#include <stdlib.h>

#include "common.h"
#include "mmix/io.h"
#include "suites/tphysmem.h"
#include "suites/tregister.h"
#include "suites/tmemhierarchy.h"
#include "suites/tsysinstrs.h"
#include "suites/tinterruptibility.h"
#include "exception.h"
#include "config.h"
#include "sim.h"
#include "test.h"

int main(void) {
	cfg_setUserMode(true);
	cfg_setRAMSize((octa)32 * 1024 * 1024 * 1024);

	// catch exceptions that are not catched by anybody else. it may occur an exception during
	// initialisation for example.
	jmp_buf env;
	int ex = setjmp(env);
	if(ex != EX_NONE)
		mprintf("Exception: %s\n",ex_toString(ex,ex_getBits()));
	else {
		ex_push(&env);
		sim_init();
		test_register("Physical memory",tPhysMem);
		test_register("Register",tRegister);
		test_register("Memory Hierarchy",tMemHierarchy);
		test_register("System instructions",tSysInstrs);
		test_register("Interruptibility",tIntrpt);
		test_start();
	}
	ex_pop();
	test_cleanup();
	sim_shutdown();
	return EXIT_SUCCESS;
}
