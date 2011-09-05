/**
 * $Id: cmds.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <string.h>
#include <assert.h>

#include "common.h"
#include "cli/lang/eval.h"
#include "cli/cmds.h"
#include "cli/cmd/print.h"
#include "cli/cmd/set.h"
#include "cli/cmd/disasm.h"
#include "cli/cmd/cont.h"
#include "cli/cmd/step.h"
#include "cli/cmd/stepout.h"
#include "cli/cmd/stepover.h"
#include "cli/cmd/trace.h"
#include "cli/cmd/break.h"
#include "cli/cmd/effects.h"
#include "cli/cmd/help.h"
#include "cli/cmd/lang.h"
#include "cli/cmd/quit.h"
#include "cli/cmd/paging.h"
#include "cli/cmd/cache.h"
#include "cli/cmd/reset.h"
#include "cli/cmd/devices.h"
#include "cli/cmd/stat.h"
#include "cli/cmd/bt.h"
#include "cli/console.h"
#include "mmix/io.h"
#include "exception.h"
#include "mmix/mem.h"

static const sCommand *cmds_getByName(const char *name);
static const sASTNode **cmds_buildCmdArgs(const sASTNode *args,size_t *argc);

static const char *curCmdName = "";
static const char *valTypes[] = {
	"int","float","string"
};

static const sCommand commands[] = {
	{
		"h",
		NULL,
		NULL,
		cli_cmd_help,
		{
			{"","Print the command-list"},
			{NULL,NULL}
		}
	},
	{
		"hl",
		NULL,
		NULL,
		cli_cmd_lang,
		{
			{"","Print the language description"},
			{NULL,NULL}
		}
	},
	{
		"p",
		NULL,
		NULL,
		cli_cmd_print,
		{
			{"<expr..>...","Print expression(s) in hexadecimal, unsigned"},
			{"<fmt> <expr..>...","Print expression(s) in <fmt> (u,d,o,x,lx,f or s)"},
			{NULL,NULL}
		}
	},
	{
		"set",
		NULL,
		NULL,
		cli_cmd_set,
		{
			{"<obj..> <expr..>","Set <obj> to <expr>"},
			{NULL,NULL}
		}
	},
	{
		"d",
		NULL,
		NULL,
		cli_cmd_disasm,
		{
			{"","Disassemble one instruction at the PC"},
			{"<addr..>","Disassemble the instruction(s) at given address(es)"},
			{NULL,NULL}
		}
	},
	{
		"s",
		NULL,
		NULL,
		cli_cmd_step,
		{
			{"","Execute 1 instruction"},
			{"<count>","Execute <count> instructions"},
			{NULL,NULL}
		}
	},
	{
		"ou",
		NULL,
		NULL,
		cli_cmd_stepout,
		{
			{"","Step out of the current function (1 time)"},
			{"<count>","Step out of the current function (<count> times)"},
			{NULL,NULL}
		}
	},
	{
		"ov",
		NULL,
		NULL,
		cli_cmd_stepover,
		{
			{"","Step over next instruction (1 time)"},
			{"<count>","Step over next instruction (<count> times)"},
			{NULL,NULL}
		}
	},
	{
		"c",
		NULL,
		NULL,
		cli_cmd_cont,
		{
			{"","Continue 1 time"},
			{"<count>","Continue <count> times"},
			{NULL,NULL}
		}
	},
	{
		"b",
		cli_cmd_breakInit,
		NULL,
		cli_cmd_break,
		{
			{"","Print breakpoints"},
			{"<addr>","Break if <addr> (virt) is accessed for mode x"},
			{"v <mode> <addr>","Break if <addr> (virt) is accessed for <mode> (rwx)"},
			{"p <mode> <addr>","Break if <addr> (phys) is accessed for <mode> (rwx)"},
			{"e <expr..>","Break if the value of <expr> changed"},
			{"e <expr..> <val..>","Break if the value of <expr> is <val>"},
			{NULL,NULL}
		}
	},
	{
		"db",
		NULL,
		NULL,
		cli_cmd_delBreak,
		{
			{"<no..>","Delete breakpoint(s) <no>"},
			{NULL,NULL}
		}
	},
	{
		"e",
		cli_cmd_effectsInit,
		NULL,
		cli_cmd_effects,
		{
			{"","Print effects of last instr. (flags=s)"},
			{"<flags>","Print effects of last instr. with additional info:\n"
					"                             s = stack, d = devices, c = caches,\n"
					"                             v = virt. addr. transl., f = effects during fetch"},
			{"on|off","Enable or disable the effects-command"},
			{NULL,NULL}
		}
	},
	{
		"tr",
		cli_cmd_traceInit,
		NULL,
		cli_cmd_trace,
		{
			{"","Print all traces"},
			{"<cmd> [<arg>...]","Execute the given command after each cpu-stop"},
			{NULL,NULL}
		}
	},
	{
		"dtr",
		NULL,
		NULL,
		cli_cmd_delTrace,
		{
			{"<no..>","Delete trace(s) <no>"},
			{NULL,NULL}
		}
	},
	{
		"dev",
		NULL,
		NULL,
		cli_cmd_devices,
		{
			{"","Print the state of all connected devices"},
			{NULL,NULL}
		}
	},
	{
		"v2p",
		NULL,
		NULL,
		cli_cmd_v2p,
		{
			{"<addr>","Translate the virtual address <addr>"},
			{NULL,NULL}
		}
	},
	{
		"itc",
		NULL,
		NULL,
		cli_cmd_itc,
		{
			{"","Print all Instruction-TC-entries"},
			{"<addr..>","Print ITC-entry for virtual address(es) <addr>"},
			{NULL,NULL}
		}
	},
	{
		"dtc",
		NULL,
		NULL,
		cli_cmd_dtc,
		{
			{"","Print all Data-TC-entries"},
			{"<addr..>","Print DTC-entry for virtual address(es) <addr>"},
			{NULL,NULL}
		}
	},
	{
		"ic",
		NULL,
		NULL,
		cli_cmd_ic,
		{
			{"","Print all instruction-cache-entries"},
			{"<addr..>","Print IC-entry for physical address(es) <addr>"},
			{NULL,NULL}
		}
	},
	{
		"dc",
		NULL,
		NULL,
		cli_cmd_dc,
		{
			{"","Print all data-cache-entries"},
			{"<addr..>","Print DC-entry for physical address(es) <addr>"},
			{NULL,NULL}
		}
	},
	{
		"st",
		cli_cmd_statInit,
		cli_cmd_statReset,
		cli_cmd_stat,
		{
			{"","Print execution statistic"},
			{"on|off","Enable and reset or disable execution stats"},
			{NULL,NULL}
		}
	},
	{
		"bt",
		cli_cmd_btInit,
		cli_cmd_btReset,
		cli_cmd_bt,
		{
			{"","Print backtrace"},
			{"<no>","Print backtrace with number <no>"},
			{NULL,NULL}
		}
	},
	{
		"btt",
		NULL,
		NULL,
		cli_cmd_btTree,
		{
			{"<file>","Print backtrace tree to file <file>"},
			{"<file> <no>","Print backtrace with number <no> to file <file>"},
			{NULL,NULL}
		}
	},
	{
		"dbt",
		NULL,
		NULL,
		cli_cmd_delBt,
		{
			{"<no>","Reset the trace with number <no>"},
			{NULL,NULL}
		}
	},
	{
		"r",
		NULL,
		NULL,
		cli_cmd_reset,
		{
			{"","Reset"},
			{NULL,NULL}
		}
	},
	{
		"q",
		NULL,
		NULL,
		cli_cmd_quit,
		{
			{"","Quit"},
			{NULL,NULL}
		}
	},
};
static volatile bool interrupted = false;

void cmds_init(void) {
	for(size_t i = 0; i < ARRAY_SIZE(commands); i++) {
		if(commands[i].init)
			commands[i].init();
	}
}

void cmds_reset(void) {
	for(size_t i = 0; i < ARRAY_SIZE(commands); i++) {
		if(commands[i].reset)
			commands[i].reset();
	}
}

void cmds_interrupt(void) {
	interrupted = true;
}

void cmds_exec(const sASTNode *node) {
	size_t argc;
	const sASTNode **argv = cmds_buildCmdArgs(node->d.cmdStmt.args,&argc);
	curCmdName = node->d.cmdStmt.name;
	// ensure that we free the memory, if an exception is thrown
	jmp_buf env;
	int ex = setjmp(env);
	if(ex != EX_NONE) {
		mem_free(argv);
		// CLI exceptions have already been printed
		if(ex != EX_CLI) {
			mprintf("Exception during command-execution: %s\n",
					ex_toString(ex,ex_getBits()));
		}
		ex_pop();
		ex_rethrow();
	}
	else {
		ex_push(&env);
		const sCommand *cmd = cmds_getByName(node->d.cmdStmt.name);
		if(cmd == NULL)
			cmds_throwEx("Unknown command '%s'\n",node->d.cmdStmt.name);
		interrupted = false;
		cmd->cmd(argc,argv);
		ex_pop();
		mem_free(argv);
	}
}

void cmds_throwEx(const char *fmt,...) {
	const sCommand *cmd = cmds_getByName(curCmdName);
	if(!cmd || fmt) {
		va_list ap;
		va_start(ap,fmt);
		mvprintf(fmt,ap);
		va_end(ap);
	}
	else {
		mprintf("Usage:\n");
		for(size_t j = 0; ; j++) {
			if(cmd->variants[j].synopsis == NULL)
				break;
			mprintf("  %-5s %-20s %s\n",cmd->name,cmd->variants[j].synopsis,cmd->variants[j].desc);
		}
	}
	ex_throw(EX_CLI,0);
}

const sCommand *cmds_get(size_t index) {
	if(index >= ARRAY_SIZE(commands))
		return NULL;
	return commands + index;
}

sCmdArg cmds_evalArg(const sASTNode *arg,int expTypes,octa offset,bool *finished) {
	bool lFinished = true;
	bool *pFin = finished ? finished : &lFinished;
	*pFin = true;
	sCmdArg val = eval_get(arg,offset,pFin);
	if(interrupted)
		*pFin = true;
	if(*pFin)
		return val;

	if(!(expTypes & val.type)) {
		static char buffer[255];
		ast_toString(arg,buffer,sizeof(buffer));
		mprintf("Invalid argument '%s': Expecting a type of: ",buffer);
		for(size_t i = 0; i < ARRAY_SIZE(valTypes); i++) {
			if(expTypes & (1 << i))
				mprintf("%s ",valTypes[i]);
		}
		mprintf("\n");
		ex_throw(EX_CLI,0);
	}
	return val;
}

static const sCommand *cmds_getByName(const char *name) {
	for(size_t i = 0; i < ARRAY_SIZE(commands); i++) {
		if(strcmp(commands[i].name,name) == 0)
			return commands + i;
	}
	return NULL;
}

static const sASTNode **cmds_buildCmdArgs(const sASTNode *args,size_t *argc) {
	size_t c = 0;
	const sASTNode **argv = NULL;
	const sASTNode *list = args;
	while(!list->d.exprList.isEmpty) {
		c++;
		list = list->d.exprList.tail;
	}
	argv = (const sASTNode**)mem_alloc(sizeof(sASTNode*) * c);
	list = args;
	for(size_t i = 0; !list->d.exprList.isEmpty; i++) {
		argv[c - (i + 1)] = list->d.exprList.head;
		list = list->d.exprList.tail;
	}
	*argc = c;
	return argv;
}
