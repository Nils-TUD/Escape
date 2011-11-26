/**
 * $Id: main.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "mmix/io.h"
#include "mmix/error.h"
#include "core/cpu.h"
#include "core/bus.h"
#include "cli/console.h"
#include "cli/lang/symmap.h"
#include "config.h"
#include "exception.h"
#include "postcmds.h"
#include "gdbstub.h"
#include "sim.h"

static const char *usage = "Usage: %s\n"
	"	[-i]			interactive mode\n"
	"	[-t <n>]		sets the number of terminals to <n>\n"
	"	[-l <prog>]		program file name\n"
	"	[-r <rom>]		ROM image file name\n"
	"	[-d <disk>]		disk image file name\n"
	"	[-o <file>]		output device file binding\n"
	"	[-c]			console present\n"
	"	[-m <size>]		the amount of main-memory in MiB\n"
	"	[-p <port>]		listen on given port for gdb\n"
	"	[--start=<addr>]	sets the PC to <addr>\n"
	"	[--script=<file>]	run the script <file> at the beginning (implies -i)\n"
	"	[--postcmds=<cmds>]	executes <cmds> before shutting down (not in interactive mode)\n"
	"	[--user]		establishes a \"user-mode environment\" at the beginning.\n"
	"				That means, the PC will be set to 0x1000, rO,rS,rK,rT and rTT will be set,\n"
	"				paging will be configured and the amount of main-memory will be 32GiB.\n"
	"	[-b]			enable backtraces\n"
	"	[-s <map1> ...]		symbols(s) for backtraces\n"
	"	[--test]			enable testmode (if enabled TRAP 0,0,0 halts the CPU)";

/* put them here, because gcc complains that it might be clobbered by longjmp */
static int gdbport = -1;
static char *script = NULL;
static char *postCmds = NULL;

int main(int argc,char **argv) {
	if(sizeof(byte) != 1 || sizeof(wyde) != 2 || sizeof(tetra) != 4 || sizeof(octa) != 8)
		error("Type-sizes are wrong. Please change the typedefs in common.h for your platform!");

	bool interactive = false;
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i],"-i") == 0)
			interactive = true;
		else if(strncmp(argv[i],"--postcmds=",sizeof("--postcmds=") - 1) == 0)
			postCmds = argv[i] + sizeof("--postcmds=") - 1;
		else if(strncmp(argv[i],"--script=",sizeof("--script=") - 1) == 0) {
			script = argv[i] + sizeof("--script=") - 1;
			interactive = true;
		}
		else if(strncmp(argv[i],"--start=",sizeof("--start=") - 1) == 0) {
			octa cpc = mstrtoo(argv[i] + sizeof("--start=") - 1,NULL,0);
			cfg_setStartAddr(cpc);
		}
		else if(strcmp(argv[i],"--user") == 0) {
			cfg_setUserMode(true);
			cfg_setStartAddr(0x1000);
			cfg_setRAMSize((octa)32 * 1024 * 1024 * 1024);
		}
		else if(strcmp(argv[i],"--test") == 0)
			cfg_setTestMode(true);
		else if(strcmp(argv[i],"-t") == 0) {
			if(i >= argc - 1)
				error(usage,argv[0]);
			cfg_setTermCount(mstrtoo(argv[i + 1],NULL,0));
			i++;
		}
		else if(strcmp(argv[i],"-l") == 0) {
			if(i >= argc - 1)
				error(usage,argv[0]);
			cfg_setProgram(argv[i + 1]);
			i++;
		}
		else if(strcmp(argv[i],"-r") == 0) {
			if(i >= argc - 1)
				error(usage,argv[0]);
			cfg_setROM(argv[i + 1]);
			cfg_setStartAddr(MSB(64) | ROM_START_ADDR);
			i++;
		}
		else if(strcmp(argv[i],"-d") == 0) {
			if(i >= argc - 1)
				error(usage,argv[0]);
			cfg_setDiskImage(argv[i + 1]);
			i++;
		}
		else if(strcmp(argv[i],"-o") == 0) {
			if(i >= argc - 1)
				error(usage,argv[0]);
			cfg_setOutputFile(argv[i + 1]);
			i++;
		}
		else if(strcmp(argv[i],"-m") == 0) {
			if(i >= argc - 1)
				error(usage,argv[0]);
			cfg_setRAMSize(mstrtoo(argv[i + 1],NULL,0) * 1024 * 1024);
			i++;
		}
		else if(strcmp(argv[i],"-c") == 0)
			cfg_setUseDspKbd(true);
		else if(strcmp(argv[i],"-p") == 0) {
			if(i >= argc - 1)
				error(usage,argv[0]);
			gdbport = atoi(argv[i + 1]);
			i++;
		}
		else if(strcmp(argv[i],"-b") == 0)
			cfg_setUseBacktracing(true);
		else if(strcmp(argv[i],"-s") == 0) {
			const char *mapNames[MAX_MAPS];
			for(int x = 0; x < MAX_MAPS; x++)
				mapNames[x] = NULL;
			if(i == argc - 1 || mapNames[0] != NULL)
				error(usage,argv[0]);
			for(int x = 0; x < MAX_MAPS && i < argc - 1; x++) {
				mapNames[x] = argv[++i];
				if(mapNames[x][0] == '-') {
					mapNames[x] = NULL;
					i--;
					break;
				}
			}
			cfg_setMaps(mapNames);
		}
		else
			error(usage,argv[0]);
	}

	// in non-interactive mode, either a program or ROM is required
	if(!interactive && cfg_getProgram() == NULL && cfg_getROM() == NULL)
		error(usage,argv[0]);

	// catch exceptions that are not catched by anybody else. it may occur an exception during
	// initialization for example.
	jmp_buf env;
	int ex = setjmp(env);
	if(ex != EX_NONE)
		mprintf("Exception: %s\n",ex_toString(ex,ex_getBits()));
	else {
		ex_push(&env);
		sim_init();
		if(gdbport != -1)
			gdbstub_init(gdbport);
		else if(interactive) {
			cons_init();
			if(script)
				cons_exec(script,true);
			cons_start();
		}
		else {
			cpu_run();
			if(postCmds)
				postcmds_perform(postCmds);
		}
	}
	ex_pop();
	sim_shutdown();

	// ignore errors raised by the gcov stuff
	fclose(stderr);
	return EXIT_SUCCESS;
}
