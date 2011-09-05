/**
 * $Id: postcmds.c 200 2011-05-08 15:40:08Z nasmussen $
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "common.h"
#include "core/register.h"
#include "core/bus.h"
#include "postcmds.h"
#include "mmix/io.h"

#define MAX_POST_CMDS	16

#define TYPE_NULL		0
#define TYPE_DREGS		1
#define TYPE_MEM		2

typedef struct {
	int type;
	union {
		struct {
			int from;
			int to;
		} regs;
		struct {
			octa from;
			octa to;
		} mem;
	} data;
} sPostCmd;

static void postcmds_exec(void);
static void postcmds_dregs(const sPostCmd *cmd);
static void postcmds_mem(const sPostCmd *cmd);
static void postcmds_parse(const char *arg);

static sPostCmd cmds[MAX_POST_CMDS + 1];

void postcmds_perform(const char *cmdspec) {
	postcmds_parse(cmdspec);
	postcmds_exec();
}

static void postcmds_exec(void) {
	for(int i = 0; cmds[i].type != TYPE_NULL; i++) {
		switch(cmds[i].type) {
			case TYPE_DREGS:
				postcmds_dregs(cmds + i);
				break;
			case TYPE_MEM:
				postcmds_mem(cmds + i);
				break;
		}
	}
}

static void postcmds_dregs(const sPostCmd *cmd) {
	for(int j = cmd->data.regs.from; j <= cmd->data.regs.to; j++)
		mprintf("$%d: %016OX\n",j,reg_get(j));
}

static void postcmds_mem(const sPostCmd *cmd) {
	for(octa addr = cmd->data.mem.from; addr <= cmd->data.mem.to; addr += 8)
		mprintf("m[%016OX]: %016OX\n",addr,bus_read(addr,false));
}

static void postcmds_parse(const char *arg) {
	int i;
	for(i = 0; i < MAX_POST_CMDS && *arg; i++) {
		switch(*arg) {
			case 'r':
				// skip "r:"
				arg += 2;
				cmds[i].type = TYPE_DREGS;
				msscanf(arg,"$%d..$%d",&cmds[i].data.regs.from,&cmds[i].data.regs.to);
				break;
			case 'm':
				// skip "m:"
				arg += 2;
				cmds[i].type = TYPE_MEM;
				msscanf(arg,"%Ox..%Ox",&cmds[i].data.mem.from,&cmds[i].data.mem.to);
				// we can't access the physical memory without alignment
				cmds[i].data.mem.from &= ~(octa)(sizeof(octa) - 1);
				break;
			default:
				assert(false);
				break;
		}

		// walk to separator or end
		while(*arg && *arg != ',')
			arg++;
		if(*arg == ',')
			arg++;
	}
	// terminate
	cmds[i].type = TYPE_NULL;
}
