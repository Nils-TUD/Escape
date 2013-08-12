/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/common.h>
#include <sys/dbg/console.h>
#include <sys/dbg/cmd/break.h>
#include <sys/task/proc.h>
#include <sys/task/smp.h>
#include <sys/util.h>
#include <string.h>

enum {
	BRK_LEN_1		= 0,
	BRK_LEN_2		= 1,
	BRK_LEN_4		= 3
};

enum {
	/* instruction execution */
	BRK_COND_EXEC	= 0,
	/* data writes */
	BRK_COND_WRITE	= 1,
	/* I/O reads or writes */
	BRK_COND_IO		= 2,
	/* read or write, but no instruction fetches */
	BRK_COND_RDWR	= 3
};

static inline uint32_t reg_get(uint32_t val,int base,int reg,int width,uint32_t mask) {
	return (val >> (base + (reg * width))) & mask;
}
static inline uint32_t reg_set(uint32_t val,int base,int reg,int width,uint32_t mask,uint32_t newval) {
	return (val & ~(mask << (base + reg * width))) | (newval << (base + (reg * width)));
}
static inline uint32_t dr7_get_len(uint32_t dr7,int reg) {
	return reg_get(dr7,18,reg,4,0x3);
}
static inline uint32_t dr7_set_len(uint32_t dr7,int reg,int len) {
	return reg_set(dr7,18,reg,4,0x3,len);
}
static inline uint32_t dr7_get_cond(uint32_t dr7,int reg) {
	return reg_get(dr7,16,reg,4,0x3);
}
static inline uint32_t dr7_set_cond(uint32_t dr7,int reg,int rw) {
	return reg_set(dr7,16,reg,4,0x3,rw);
}
static inline uint32_t dr7_get_glb_en(uint32_t dr7,int reg) {
	return reg_get(dr7,1,reg,2,0x1);
}
static inline uint32_t dr7_set_glb_en(uint32_t dr7,int reg,int en) {
	return reg_set(dr7,1,reg,2,0x1,en);
}

static inline uint32_t dr6_get_cond(uint32_t dr6,int reg) {
	return reg_get(dr6,0,reg,1,0x1);
}

static const char *lensNames[] = {
	"1","2","?","4"
};
static int lens[] = {
	-1, BRK_LEN_1, BRK_LEN_2, -1, BRK_LEN_4
};

static const char *condNames[] = {
	"x","w","io","rw"
};

struct Breakpoint {
	uint32_t addr;
	int len;
	int cond;
	int enabled;
};

static bool changed = false;
static Breakpoint breakpoints[4];

static uint32_t getDR(int r) {
	switch(r) {
		case 0: return CPU::getDR0();
		case 1: return CPU::getDR1();
		case 2: return CPU::getDR2();
		case 3: return CPU::getDR3();
	}
	assert(false);
	A_UNREACHED;
}
static void setDR(int r,uint32_t val) {
	switch(r) {
		case 0: CPU::setDR0(val); break;
		case 1: CPU::setDR1(val); break;
		case 2: CPU::setDR2(val); break;
		case 3: CPU::setDR3(val); break;
	}
}

static int findFreeReg() {
	for(int i = 0; i < 4; ++i) {
		if(getDR(i) == 0)
			return i;
	}
	return -1;
}

static void usage(OStream &os) {
	os.writef("Usage: break [l|s|d|en|dis|i]\n");
	os.writef("  l                            - list breakpoints\n");
	os.writef("  s <addr> [<len>] [x|w|io|rw] - set breakpoint\n");
	os.writef("  d <no>                       - delete <no>\n");
	os.writef("  en <no>                      - enable <no>\n");
	os.writef("  dis <no>                     - disable <no>\n");
	os.writef("  i                            - show information\n");
}

static void set(int reg,uint32_t addr,int len,int cond,int en) {
	setDR(reg,addr);
	uint32_t dr7 = CPU::getDR7();
	dr7 = dr7_set_len(dr7,reg,len);
	dr7 = dr7_set_cond(dr7,reg,cond);
	dr7 = dr7_set_glb_en(dr7,reg,en);
	breakpoints[reg].addr = addr;
	breakpoints[reg].len = len;
	breakpoints[reg].cond = cond;
	breakpoints[reg].enabled = en;
	CPU::setDR7(dr7);
	changed = true;
}

static void sync_breakpoints() {
	for(int i = 0; i < 4; ++i)
		set(i,breakpoints[i].addr,breakpoints[i].len,breakpoints[i].cond,breakpoints[i].enabled);
}

static int getRegNo(OStream &os,size_t argc,char **argv) {
	if(argc < 3) {
		usage(os);
		return -1;
	}

	int reg = atoi(argv[2]);
	if(reg < 0 || reg >= 4) {
		os.writef("Invalid register number\n");
		return -1;
	}
	return reg;
}

static void printBreakpoint(OStream &os,int i,uint32_t addr) {
	uint32_t dr7 = CPU::getDR7();
	os.writef("Breakpoint %d: addr=%p len=%s cond=%s en=%d\n",
			i,addr,lensNames[dr7_get_len(dr7,i)],condNames[dr7_get_cond(dr7,i)],
			dr7_get_glb_en(dr7,i));
}

static int set_breakpoint(OStream &os,size_t argc,char **argv) {
	if(argc < 3) {
		usage(os);
		return 0;
	}

	int reg = findFreeReg();
	if(reg == -1) {
		os.writef("All breakpoints are in use\n");
		return 0;
	}

	uint32_t addr = strtoul(argv[2],NULL,0);
	if(addr == 0) {
		os.writef("The break-address can't be 0\n");
		return 0;
	}

	int len = argc > 3 ? atoi(argv[3]) : 4;
	if(len < 0 || len >= (int)ARRAY_SIZE(lens) || lens[len] == -1) {
		usage(os);
		return 0;
	}
	len = lens[len];

	int cond = BRK_COND_EXEC;
	if(argc > 4) {
		if(strcmp(argv[4],"w") == 0)
			cond = BRK_COND_WRITE;
		else if(strcmp(argv[4],"io") == 0)
			cond = BRK_COND_IO;
		else if(strcmp(argv[4],"rw") == 0)
			cond = BRK_COND_RDWR;
		else if(strcmp(argv[4],"x") != 0) {
			usage(os);
			return 0;
		}
	}

	set(reg,addr,len,cond,1);
	printBreakpoint(os,reg,getDR(reg));
	return 0;
}

static int delete_breakpoint(OStream &os,size_t argc,char **argv) {
	int reg = getRegNo(os,argc,argv);
	if(reg < 0)
		return 0;
	set(reg,0,BRK_LEN_1,BRK_COND_EXEC,0);
	printBreakpoint(os,reg,getDR(reg));
	return 0;
}

static int endis_breakpoint(OStream &os,size_t argc,char **argv,int en) {
	int reg = getRegNo(os,argc,argv);
	if(reg < 0)
		return 0;
	set(reg,breakpoints[reg].addr,breakpoints[reg].len,breakpoints[reg].cond,en);
	printBreakpoint(os,reg,getDR(reg));
	return 0;
}

static int show_info(OStream &os,size_t,char **) {
	bool triggered = false;
	uint32_t dr6 = CPU::getDR6();
	uint32_t dr7 = CPU::getDR7();
	for(int i = 0; i < 4; ++i) {
		if(dr7_get_glb_en(dr7,i) && dr6_get_cond(dr6,i)) {
			os.writef("Breakpoint %d triggered because of '%s' access for address %p\n",
					i,condNames[dr7_get_cond(dr7,i)],getDR(i));
			triggered = true;
		}
	}
	if(triggered)
		os.writef("\n");
	return 0;
}

int cons_cmd_break(OStream &os,size_t argc,char **argv) {
	if(Console::isHelp(argc,argv) || argc > 5) {
		usage(os);
		return 0;
	}

	int res = 0;
	changed = false;
	if(argc == 1 || strcmp(argv[1],"l") == 0) {
		for(int i = 0; i < 4; ++i)
			printBreakpoint(os,i,getDR(i));
	}
	else if(strcmp(argv[1],"s") == 0)
		res = set_breakpoint(os,argc,argv);
	else if(strcmp(argv[1],"d") == 0)
		res = delete_breakpoint(os,argc,argv);
	else if(strcmp(argv[1],"en") == 0)
		res = endis_breakpoint(os,argc,argv,1);
	else if(strcmp(argv[1],"dis") == 0)
		res = endis_breakpoint(os,argc,argv,0);
	else if(strcmp(argv[1],"i") == 0)
		res = show_info(os,argc,argv);
	else
		usage(os);

	/* if we have changed the breakpoints, let all other CPUs set the breakpoints as well */
	if(changed)
		SMP::callbackOthers(sync_breakpoints);
	return res;
}
