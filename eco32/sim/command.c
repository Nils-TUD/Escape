/*
 * command.c -- command interpreter
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "common.h"
#include "console.h"
#include "error.h"
#include "except.h"
#include "command.h"
#include "asm.h"
#include "disasm.h"
#include "cpu.h"
#include "mmu.h"
#include "memory.h"
#include "timer.h"
#include "dspkbd.h"
#include "term.h"
#include "disk.h"
#include "graph.h"
#include "trace/trace.h"


#define MAX_TOKENS	10


static Bool quit;


void help(void) {
  cPrintf("valid commands are:\n");
  cPrintf("  +  <num1> <num2>  add and subtract <num1> and <num2>\n");
  cPrintf("  a                 assemble starting at PC\n");
  cPrintf("  a  <addr>         assemble starting at <addr>\n");
  cPrintf("  u                 unassemble 16 instrs starting at PC\n");
  cPrintf("  u  <addr>         unassemble 16 instrs starting at <addr>\n");
  cPrintf("  u  <addr> <cnt>   unassemble <cnt> instrs starting at <addr>\n");
  cPrintf("  b                 reset break\n");
  cPrintf("  b  <addr>         set break at <addr>. You may also specify the function-name\n");
  cPrintf("                    if tracing is enabled\n");
  cPrintf("  wb                reset write-break\n");
  cPrintf("  wb <addr>         break if <addr> has been written\n");
  cPrintf("  c                 continue execution\n");
  cPrintf("  c  <cnt>          continue execution <cnt> times\n");
  cPrintf("  s                 single-step one instruction\n");
  cPrintf("  s  <cnt>          single-step <cnt> instructions\n");
  cPrintf("  ou                step out of the current function / interrupt\n");
  cPrintf("  ou <cnt>          step out of <cnt> functions / interrupts\n");
  cPrintf("  ov                step over. That means the CPU breaks as soon as\n");
  cPrintf("                    the same stack-level is reached again\n");
  cPrintf("  ov <cnt>          step over the next <cnt> instructions.\n");
  cPrintf("  @                 show PC\n");
  cPrintf("  @  <addr>         set PC to <addr>\n");
  cPrintf("  p                 show PSW\n");
  cPrintf("  p  <data>         set PSW to <data>\n");
  cPrintf("  r                 show all registers\n");
  cPrintf("  r  <reg>          show register <reg>\n");
  cPrintf("  r  <reg> <data>   set register <reg> to <data>\n");
  cPrintf("  d                 dump 256 bytes starting at PC\n");
  cPrintf("  d  <addr>         dump 256 bytes starting at <addr>\n");
  cPrintf("  d  <addr> <cnt>   dump <cnt> bytes starting at <addr>\n");
  cPrintf("  mw                show memory word at PC\n");
  cPrintf("  mw <addr>         show memory word at <addr>\n");
  cPrintf("  mw <addr> <data>  set memory word at <addr> to <data>\n");
  cPrintf("  mh                show memory halfword at PC\n");
  cPrintf("  mh <addr>         show memory halfword at <addr>\n");
  cPrintf("  mh <addr> <data>  set memory halfword at <addr> to <data>\n");
  cPrintf("  mb                show memory byte at PC\n");
  cPrintf("  mb <addr>         show memory byte at <addr>\n");
  cPrintf("  mb <addr> <data>  set memory byte at <addr> to <data>\n");
  cPrintf("  t                 show TLB contents\n");
  cPrintf("  t  <i>            show TLB contents at <i>\n");
  cPrintf("  t  <i> p <data>   set TLB contents at <i> to page <data>\n");
  cPrintf("  t  <i> f <data>   set TLB contents at <i> to frame <data>\n");
  cPrintf("  i                 initialize hardware\n");
  cPrintf("  bt [<no>]         displayes the stack-trace (tracing has to be enabled!)\n");
  cPrintf("  btt <file> [<no>] writes the complete call-structure to <file>\n");
  cPrintf("  q                 quit simulator\n");
}


static char *cause[32] = {
  /*  0 */  "terminal 0 transmitter interrupt",
  /*  1 */  "terminal 0 receiver interrupt",
  /*  2 */  "terminal 1 transmitter interrupt",
  /*  3 */  "terminal 1 receiver interrupt",
  /*  4 */  "keyboard interrupt",
  /*  5 */  "unknown interrupt",
  /*  6 */  "unknown interrupt",
  /*  7 */  "unknown interrupt",
  /*  8 */  "disk interrupt",
  /*  9 */  "unknown interrupt",
  /* 10 */  "unknown interrupt",
  /* 11 */  "unknown interrupt",
  /* 12 */  "unknown interrupt",
  /* 13 */  "unknown interrupt",
  /* 14 */  "timer interrupt",
  /* 15 */  "unknown interrupt",
  /* 16 */  "bus timeout exception",
  /* 17 */  "illegal instruction exception",
  /* 18 */  "privileged instruction exception",
  /* 19 */  "divide instruction exception",
  /* 20 */  "trap instruction exception",
  /* 21 */  "TLB miss exception",
  /* 22 */  "TLB write exception",
  /* 23 */  "TLB invalid exception",
  /* 24 */  "illegal address exception",
  /* 25 */  "privileged address exception",
  /* 26 */  "unknown exception",
  /* 27 */  "unknown exception",
  /* 28 */  "unknown exception",
  /* 29 */  "unknown exception",
  /* 30 */  "unknown exception",
  /* 31 */  "unknown exception"
};


char *exceptionToString(int exception) {
  if (exception < 0 ||
      exception >= sizeof(cause)/sizeof(cause[0])) {
    error("exception number out of bounds");
  }
  return cause[exception];
}


static Bool getHexNumber(char *str, Word *valptr) {
  char *end;

  *valptr = strtoul(str, &end, 16);
  return *end == '\0';
}


static Bool getDecNumber(char *str, int *valptr) {
  char *end;

  *valptr = strtoul(str, &end, 10);
  return *end == '\0';
}


static void showPC(void) {
  Word pc, psw, funcStart;
  Word instr;

  pc = cpuGetPC();
  psw = cpuGetPSW();
  instr = mmuReadWord(pc, psw & PSW_UM);
  const char *func = traceGetNearestFuncName(pc,&funcStart);
  cPrintf("PC   %08X (%-28.28s) [PC]   %08X   %s\n",
          pc, func, instr, disasm(instr, pc));
}


static void showBreakAndTotal(void) {
  Word brk;
  Word tot;

  brk = cpuGetBreak();
  tot = cpuGetTotal();
  cPrintf("Brk  ");
  if (cpuTestBreak()) {
    cPrintf("%08X", brk);
  } else {
    cPrintf("--------");
  }
  cPrintf("     Total  %08X   instructions\n", tot);
}


static void showWriteBreakAndTotal(void) {
  Word brk;
  Word tot;

  brk = cpuGetWriteBreak();
  tot = cpuGetTotal();
  cPrintf("WBrk ");
  if (cpuTestWriteBreak()) {
    cPrintf("%08X", brk);
  } else {
    cPrintf("--------");
  }
  cPrintf("     Total  %08X   instructions\n", tot);
}


static void showIRQ(void) {
  Word irq;
  int i;

  irq = cpuGetIRQ();
  cPrintf("IRQ                            ");
  for (i = 15; i >= 0; i--) {
    cPrintf("%c", irq & (1 << i) ? '1' : '0');
  }
  cPrintf("\n");
}


static void showPSW(void) {
  Word psw;
  int i;

  psw = cpuGetPSW();
  cPrintf("     xxxx  V  UPO  IPO  IACK   MASK\n");
  cPrintf("PSW  ");
  for (i = 31; i >= 0; i--) {
    if (i == 27 || i == 26 || i == 23 || i == 20 || i == 15) {
      cPrintf("  ");
    }
    cPrintf("%c", psw & (1 << i) ? '1' : '0');
  }
  cPrintf("\n");
}


static void doArith(char *tokens[], int n) {
  Word num1, num2, num3, num4;

  if (n == 3) {
    if (!getHexNumber(tokens[1], &num1)) {
      cPrintf("illegal first number\n");
      return;
    }
    if (!getHexNumber(tokens[2], &num2)) {
      cPrintf("illegal second number\n");
      return;
    }
    num3 = num1 + num2;
    num4 = num1 - num2;
    cPrintf("add = %08X, sub = %08X\n", num3, num4);
  } else {
    help();
  }
}


static void doAssemble(char *tokens[], int n) {
  Word addr;
  Word psw;
  char prompt[30];
  char *line;
  char *msg;
  Word instr;

  if (n == 1) {
    addr = cpuGetPC();
  } else if (n == 2) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
  } else {
    help();
    return;
  }
  addr &= ~0x00000003;
  psw = cpuGetPSW();
  while (1) {
    sprintf(prompt, "ASM @ %08X: ", addr);
    line = cGetLine(prompt);
    if (*line == '\0' || *line == '\n') {
      break;
    }
    cAddHist(line);
    msg = asmInstr(line, addr, &instr);
    if (msg != NULL) {
      cPrintf("%s\n", msg);
    } else {
      mmuWriteWord(addr, instr, psw & PSW_UM);
      addr += 4;
    }
  }
}


static void doUnassemble(char *tokens[], int n) {
  Word addr, count;
  Word psw;
  int i;
  Word instr;

  if (n == 1) {
    addr = cpuGetPC();
    count = 16;
  } else if (n == 2) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    count = 16;
  } else if (n == 3) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    if (!getHexNumber(tokens[2], &count)) {
      cPrintf("illegal count\n");
      return;
    }
    if (count == 0) {
      return;
    }
  } else {
    help();
    return;
  }
  addr &= ~0x00000003;
  psw = cpuGetPSW();
  for (i = 0; i < count; i++) {
    instr = mmuReadWord(addr, psw & PSW_UM);
    cPrintf("%08X:  %08X    %s\n",
            addr, instr, disasm(instr, addr));
    if (addr + 4 < addr) {
      /* wrap-around */
      break;
    }
    addr += 4;
  }
}


static void doBreak(char *tokens[], int n) {
  Word addr;

  if (n == 1) {
    cpuResetBreak();
    showBreakAndTotal();
  } else if (n == 2) {
    /* if tracing is enabled, try to find the address by name */
	if(traceIsEnabled()) {
		addr = traceGetAddress(tokens[1]);
		if(addr) {
			addr &= ~0x00000003;
			cpuSetBreak(addr);
			showBreakAndTotal();
			return;
		}
	}

    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    addr &= ~0x00000003;
    cpuSetBreak(addr);
    showBreakAndTotal();
  } else {
    help();
  }
}


static void doWriteBreak(char *tokens[], int n) {
  Word addr;

  if (n == 1) {
    cpuResetWriteBreak();
    showWriteBreakAndTotal();
  } else if (n == 2) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    cpuSetWriteBreak(addr);
    showWriteBreakAndTotal();
  } else {
    help();
  }
}


static void doContinue(char *tokens[], int n) {
  Word count, i;
  Word addr;

  if (n == 1) {
    count = 1;
  } else if (n == 2) {
    if (!getHexNumber(tokens[1], &count) || count == 0) {
      cPrintf("illegal count\n");
      return;
    }
  } else {
    help();
    return;
  }
  cPrintf("CPU is running, press ^C to interrupt...\n");
  for (i = 0; i < count; i++) {
    cpuRun();
  }
  addr = cpuGetPC();
  cPrintf("Break at %08X\n", addr);
  showPC();
}


static void doStep(char *tokens[], int n) {
  Word count, i;

  if (n == 1) {
    count = 1;
  } else if (n == 2) {
    if (!getHexNumber(tokens[1], &count) || count == 0) {
      cPrintf("illegal count\n");
      return;
    }
  } else {
    help();
    return;
  }
  for (i = 0; i < count; i++) {
    cpuStep();
  }
  showPC();
}

static void doStepOut(char* tokens[], int n) {
	int level = traceGetStackLevel();
	unsigned int count;
	if(level <= 0) {
		cPrintf("illegal level (%d)\n",level);
		return;
	}

	/* determine count */
	if(n == 1) {
		count = 1;
	}
	else if(n == 2) {
		if (!getHexNumber(tokens[1], &count) || count == 0 || count > level) {
			cPrintf("illegal count\n");
			return;
	    }
	}
	else {
		help();
		return;
	}

	/* run to the previous level */
	cpuSetStackLevelBreak(level - count);
	cPrintf("CPU is running, press ^C to interrupt...\n");
	cpuRun();
	showPC();
}

static void doStepOver(char* tokens[], int n) {
	int i;
	unsigned int count;

	/* determine count */
	if(n == 1) {
		count = 1;
	}
	else if(n == 2) {
		if (!getHexNumber(tokens[1], &count) || count == 0) {
			cPrintf("illegal count\n");
			return;
	    }
	}
	else {
		help();
		return;
	}

	cPrintf("CPU is running, press ^C to interrupt...\n");
	for(i = 0; i < count; i++) {
		/* run until we reach the current level again */
		cpuSetStackLevelBreak(traceGetStackLevel());
		cpuRun();
	}
	showPC();
}

static void doPC(char *tokens[], int n) {
  Word addr;

  if (n == 1) {
    showPC();
  } else if (n == 2) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    addr &= ~0x00000003;
    cpuSetPC(addr);
    showPC();
  } else {
    help();
  }
}


static void explainPSW(Word data) {
  int i;

  cPrintf("interrupt vector                   : %s (%s)\n",
          data & PSW_V ? "on " : "off",
          data & PSW_V ? "RAM" : "ROM");
  cPrintf("current user mode                  : %s (%s)\n",
          data & PSW_UM ? "on " : "off",
          data & PSW_UM ? "user" : "kernel");
  cPrintf("previous user mode                 : %s (%s)\n",
          data & PSW_PUM ? "on " : "off",
          data & PSW_PUM ? "user" : "kernel");
  cPrintf("old user mode                      : %s (%s)\n",
          data & PSW_OUM ? "on " : "off",
          data & PSW_OUM ? "user" : "kernel");
  cPrintf("current interrupt enable           : %s (%s)\n",
          data & PSW_IE ? "on " : "off",
          data & PSW_IE ? "enabled" : "disabled");
  cPrintf("previous interrupt enable          : %s (%s)\n",
          data & PSW_PIE ? "on " : "off",
          data & PSW_PIE ? "enabled" : "disabled");
  cPrintf("old interrupt enable               : %s (%s)\n",
          data & PSW_OIE ? "on " : "off",
          data & PSW_OIE ? "enabled" : "disabled");
  cPrintf("last interrupt acknowledged        : %02X  (%s)\n",
          (data & PSW_PRIO_MASK) >> 16,
          exceptionToString((data & PSW_PRIO_MASK) >> 16));
  for (i = 15; i >= 0; i--) {
    cPrintf("%-35s: %s (%s)\n",
            exceptionToString(i),
            data & (1 << i) ? "on " : "off",
            data & (1 << i) ? "enabled" : "disabled");
  }
}


static void doPSW(char *tokens[], int n) {
  Word data;

  if (n == 1) {
    data = cpuGetPSW();
    showPSW();
    showIRQ();
    explainPSW(data);
  } else if (n == 2) {
    if (!getHexNumber(tokens[1], &data)) {
      cPrintf("illegal data\n");
      return;
    }
    data &= 0x0FFFFFFF;
    cpuSetPSW(data);
    showPSW();
    showIRQ();
    explainPSW(data);
  } else {
    help();
  }
}


static void doRegister(char *tokens[], int n) {
  int i, j;
  int reg;
  Word data;

  if (n == 1) {
    for (i = 0; i < 8; i++) {
      for (j = 0; j < 4; j++) {
        reg = 8 * j + i;
        data = cpuGetReg(reg);
        cPrintf("$%-2d  %08X     ", reg, data);
      }
      cPrintf("\n");
    }
    showPSW();
    showIRQ();
    showBreakAndTotal();
    showPC();
  } else if (n == 2) {
    if (!getDecNumber(tokens[1], &reg) || reg < 0 || reg >= 32) {
      cPrintf("illegal register number\n");
      return;
    }
    data = cpuGetReg(reg);
    cPrintf("$%-2d  %08X\n", reg, data);
  } else if (n == 3) {
    if (!getDecNumber(tokens[1], &reg) || reg < 0 || reg >= 32) {
      cPrintf("illegal register number\n");
      return;
    }
    if (!getHexNumber(tokens[2], &data)) {
      cPrintf("illegal data\n");
      return;
    }
    cpuSetReg(reg, data);
  } else {
    help();
  }
}


static void doDump(char *tokens[], int n) {
  Word addr, count;
  Word psw;
  Word lo, hi, curr;
  int lines, i, j;
  Word tmp;
  Byte c;

  if (n == 1) {
    addr = cpuGetPC();
    count = 16 * 16;
  } else if (n == 2) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    count = 16 * 16;
  } else if (n == 3) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    if (!getHexNumber(tokens[2], &count)) {
      cPrintf("illegal count\n");
      return;
    }
    if (count == 0) {
      return;
    }
  } else {
    help();
    return;
  }
  psw = cpuGetPSW();
  lo = addr & ~0x0000000F;
  hi = addr + count - 1;
  if (hi < lo) {
    /* wrap-around */
    hi = 0xFFFFFFFF;
  }
  lines = (hi - lo + 16) >> 4;
  curr = lo;
  for (i = 0; i < lines; i++) {
    cPrintf("%08X:  ", curr);
    for (j = 0; j < 16; j++) {
      tmp = curr + j;
      if (tmp < addr || tmp > hi) {
        cPrintf("  ");
      } else {
        c = mmuReadByte(tmp, psw & PSW_UM);
        cPrintf("%02X", c);
      }
      cPrintf(" ");
    }
    cPrintf("  ");
    for (j = 0; j < 16; j++) {
      tmp = curr + j;
      if (tmp < addr || tmp > hi) {
        cPrintf(" ");
      } else {
        c = mmuReadByte(tmp, psw & PSW_UM);
        if (c >= 32 && c <= 126) {
          cPrintf("%c", c);
        } else {
          cPrintf(".");
        }
      }
    }
    cPrintf("\n");
    curr += 16;
  }
}


static void doMemoryWord(char *tokens[], int n) {
  Word psw;
  Word addr;
  Word data;
  Word tmpData;

  psw = cpuGetPSW();
  if (n == 1) {
    addr = cpuGetPC();
    data = mmuReadWord(addr, psw & PSW_UM);
    cPrintf("%08X:  %08X\n", addr, data);
  } else if (n == 2) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    data = mmuReadWord(addr, psw & PSW_UM);
    cPrintf("%08X:  %08X\n", addr, data);
  } else if (n == 3) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    if (!getHexNumber(tokens[2], &tmpData)) {
      cPrintf("illegal data\n");
      return;
    }
    data = tmpData;
    mmuWriteWord(addr, data, psw & PSW_UM);
  } else {
    help();
  }
}


static void doMemoryHalf(char *tokens[], int n) {
  Word psw;
  Word addr;
  Half data;
  Word tmpData;

  psw = cpuGetPSW();
  if (n == 1) {
    addr = cpuGetPC();
    data = mmuReadHalf(addr, psw & PSW_UM);
    cPrintf("%08X:  %04X\n", addr, data);
  } else if (n == 2) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    data = mmuReadHalf(addr, psw & PSW_UM);
    cPrintf("%08X:  %04X\n", addr, data);
  } else if (n == 3) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    if (!getHexNumber(tokens[2], &tmpData)) {
      cPrintf("illegal data\n");
      return;
    }
    data = (Half) tmpData;
    mmuWriteHalf(addr, data, psw & PSW_UM);
  } else {
    help();
  }
}


static void doMemoryByte(char *tokens[], int n) {
  Word psw;
  Word addr;
  Byte data;
  Word tmpData;

  psw = cpuGetPSW();
  if (n == 1) {
    addr = cpuGetPC();
    data = mmuReadByte(addr, psw & PSW_UM);
    cPrintf("%08X:  %02X\n", addr, data);
  } else if (n == 2) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    data = mmuReadByte(addr, psw & PSW_UM);
    cPrintf("%08X:  %02X\n", addr, data);
  } else if (n == 3) {
    if (!getHexNumber(tokens[1], &addr)) {
      cPrintf("illegal address\n");
      return;
    }
    if (!getHexNumber(tokens[2], &tmpData)) {
      cPrintf("illegal data\n");
      return;
    }
    data = (Byte) tmpData;
    mmuWriteByte(addr, data, psw & PSW_UM);
  } else {
    help();
  }
}


static void doTLB(char *tokens[], int n) {
  int index;
  TLB_Entry tlbEntry;
  Word data;

  if (n == 1) {
    for (index = 0; index < TLB_SIZE; index++) {
      tlbEntry = mmuGetTLB(index);
      cPrintf("TLB[%02d]    page  %08X    frame  %08X  %c  %c\n",
              index, tlbEntry.page, tlbEntry.frame,
              tlbEntry.write ? 'w' : '-',
              tlbEntry.valid ? 'v' : '-');
    }
    cPrintf("Index(1)   %08X\n", mmuGetIndex());
    cPrintf("EntryHi(2) %08X\n", mmuGetEntryHi());
    cPrintf("EntryLo(3) %08X\n", mmuGetEntryLo());
    cPrintf("BadAddr(4) %08X\n", mmuGetBadAddr());
  } else if (n == 2) {
    if (!getDecNumber(tokens[1], &index) || index < 0 || index >= TLB_SIZE) {
      cPrintf("illegal TLB index\n");
      return;
    }
    tlbEntry = mmuGetTLB(index);
    cPrintf("TLB[%02d]    page  %08X    frame  %08X  %c  %c\n",
            index, tlbEntry.page, tlbEntry.frame,
            tlbEntry.write ? 'w' : '-',
            tlbEntry.valid ? 'v' : '-');
  } else if (n == 3) {
    help();
  } else if (n == 4) {
    if (!getDecNumber(tokens[1], &index) || index < 0 || index >= TLB_SIZE) {
      cPrintf("illegal TLB index\n");
      return;
    }
    if (!getHexNumber(tokens[3], &data)) {
      cPrintf("illegal data\n");
      return;
    }
    tlbEntry = mmuGetTLB(index);
    if (strcmp(tokens[2], "p") == 0) {
      tlbEntry.page = data & PAGE_MASK;
    } else
    if (strcmp(tokens[2], "f") == 0) {
      tlbEntry.frame = data & PAGE_MASK;
      tlbEntry.write = data & TLB_WRITE ? true : false;
      tlbEntry.valid = data & TLB_VALID ? true : false;
    } else {
      cPrintf("TLB selector is not one of 'p' or 'f'\n");
      return;
    }
    mmuSetTLB(index, tlbEntry);
    cPrintf("TLB[%02d]    page  %08X    frame  %08X  %c  %c\n",
            index, tlbEntry.page, tlbEntry.frame,
            tlbEntry.write ? 'w' : '-',
            tlbEntry.valid ? 'v' : '-');
  } else {
    help();
  }
}


static void doInit(char *tokens[], int n) {
  if (n == 1) {
    timerReset();
    displayReset();
    keyboardReset();
    termReset();
    diskReset();
    graphReset();
    memoryReset();
    mmuReset();
    cpuReset();
    traceReset();
  } else {
    help();
  }
}


static void doQuit(char *tokens[], int n) {
  if (n == 1) {
    quit = true;
  } else {
    help();
  }
}


typedef struct {
  char *name;
  void (*cmdProc)(char *tokens[], int n);
} Command;


static Command commands[] = {
  { "+",  doArith },
  { "a",  doAssemble },
  { "u",  doUnassemble },
  { "b",  doBreak },
  { "wb", doWriteBreak },
  { "c",  doContinue },
  { "s",  doStep },
  {	"ou", doStepOut },
  {	"ov", doStepOver },
  { "@",  doPC },
  { "p",  doPSW },
  { "r",  doRegister },
  { "d",  doDump },
  { "mw", doMemoryWord },
  { "mh", doMemoryHalf },
  { "mb", doMemoryByte },
  { "t",  doTLB },
  { "i",  doInit },
  { "q",  doQuit },
  { "bt", doSimpleTrace },
  { "btt", doTreeTrace },
};


static Bool doCommand(char *line) {
  char *tokens[MAX_TOKENS];
  int n;
  char *p;
  int i;

  n = 0;
  p = strtok(line, " \t\n");
  while (p != NULL) {
    if (n == MAX_TOKENS) {
      cPrintf("too many tokens on line\n");
      return false;
    }
    tokens[n++] = p;
    p = strtok(NULL, " \t\n");
  }
  if (n == 0) {
    return false;
  }
  quit = false;
  for (i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
    if (strcmp(commands[i].name, tokens[0]) == 0) {
      (*commands[i].cmdProc)(tokens, n);
      return quit;
    }
  }
  help();
  return false;
}


static char *article(char firstLetterOfNoun) {
  switch (firstLetterOfNoun) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
      return "An";
    default:
      return "A";
  }
}


static void interactiveException(int exception) {
  char *what;

  what = exceptionToString(exception);
  cPrintf("\n");
  cPrintf("NOTE: %s %s occurred while executing the command.\n",
          article(*what), what);
  cPrintf("      This event will not alter the state of the CPU.\n");
}


Bool execCommand(char *line) {
  jmp_buf myEnvironment;
  int exception;
  Bool quit;

  exception = setjmp(myEnvironment);
  if (exception == 0) {
    /* initialization */
    pushEnvironment(&myEnvironment);
    quit = doCommand(line);
  } else {
    /* an exception was thrown */
    interactiveException(exception);
    quit = false;
  }
  popEnvironment();
  return quit;
}
