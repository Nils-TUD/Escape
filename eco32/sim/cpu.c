/*
 * cpu.c -- CPU simulation
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "common.h"
#include "console.h"
#include "error.h"
#include "except.h"
#include "instr.h"
#include "cpu.h"
#include "mmu.h"
#include "timer.h"
#include "trace/trace.h"


/**************************************************************/


#define RR(n)		r[n]
#define WR(n,d)		((void) ((n) != 0 ? r[n] = (d) : (d)))

#define V		(psw & PSW_V)
#define UM		(psw & PSW_UM)
#define PUM		(psw & PSW_PUM)
#define OUM		(psw & PSW_OUM)
#define IE		(psw & PSW_IE)
#define PIE		(psw & PSW_PIE)
#define OIE		(psw & PSW_OIE)


/**************************************************************/


static Bool debugIRQ = false;	/* set to true if debugging IRQs */

static Word pc;			/* program counter */
static Word psw;		/* processor status word */
static Word r[32];		/* general purpose registers */

static unsigned long long instrCount;		/* counts instrs for timer tick */
static unsigned irqPending;	/* one bit for each pending IRQ */

static Bool breakSet;		/* breakpoint set if true */
static Word breakAddr;		/* if breakSet, this is where */

static Bool wrBreakSet;		/* write-breakpoint set if true */
static Word wrBreakAddr;	/* if wrBreakSet, the virtual address */

static Word total;		/* counts total number of instrs */

static Bool run;		/* CPU runs continuously if true */

static Word startAddr;		/* start of ROM (or start of RAM, */
				/* in case a program was loaded) */

/* for step-over and step-out */
static int stackLevelBreak  = -1;
/* will be 1 if the CPU should break */
static int stackLevelReached = 0;

/**************************************************************/


static Bool isWriteBreakAddr(Word addr, int opcode) {
	Word baddr;
	if (!wrBreakSet)
		return false;

	if(opcode == OP_STW)
		baddr = wrBreakAddr & ~0x3;
	else if(opcode == OP_STH)
		baddr = wrBreakAddr & ~0x1;
	else
		baddr = wrBreakAddr;
	if(baddr >= 0xC0000000 && addr < 0xC0000000) {
		TLB_Entry e;
		int i;
		for(i = 0; i < TLB_SIZE; i++) {
			e = mmuGetTLB(i);
			if(e.valid && e.page == (addr & PAGE_MASK)) {
				addr = (e.frame | 0xC0000000) | (addr & OFFSET_MASK);
				break;
			}
		}
	}
	return baddr == addr;
}


static void handleRealTimeTasks(void) {
  /* handle 'real-time' tasks */
  if ((++instrCount % INSTRS_PER_MSEC) == 0) {
    timerTick();
  }
}


static void handleInterrupts(void) {
  unsigned irqMask;
  unsigned irqSeen;
  int priority;

  /* handle exceptions and interrupts */
  if (irqPending == 0) {
    /* no exception or interrupt pending */
    return;
  }
  /* at least one exception or interrupt is pending */
  irqMask = ~PSW_IRQ_MASK | (psw & PSW_IRQ_MASK);
  if (debugIRQ) {
    cPrintf("**** IRQ  = 0x%08X ****\n", irqPending);
    cPrintf("**** MASK = 0x%08X ****\n", irqMask);
  }
  irqSeen = irqPending & irqMask;
  if (irqSeen == 0) {
    /* none that gets through */
    return;
  }
  /* determine the one with the highest priority */
  for (priority = 31; priority >= 0; priority--) {
    if ((irqSeen & ((unsigned) 1 << priority)) != 0) {
      /* highest priority among visible ones found */
      break;
    }
  }
  /* acknowledge exception, or interrupt if enabled */
  if (priority >= 16 || IE != 0) {
    if (priority >= 16) {
      /* clear corresponding bit in irqPending vector */
      /* only done for exceptions, since interrupts are level-sensitive */
      irqPending &= ~((unsigned) 1 << priority);
    }
    /* copy and reset interrupt enable bit in PSW */
    if (PIE != 0) {
      psw |= PSW_OIE;
    } else {
      psw &= ~PSW_OIE;
    }
    if (IE != 0) {
      psw |= PSW_PIE;
    } else {
      psw &= ~PSW_PIE;
    }
    psw &= ~PSW_IE;
    /* copy and reset user mode enable bit in PSW */
    if (PUM != 0) {
      psw |= PSW_OUM;
    } else {
      psw &= ~PSW_OUM;
    }
    if (UM != 0) {
      psw |= PSW_PUM;
    } else {
      psw &= ~PSW_PUM;
    }
    psw &= ~PSW_UM;
    /* reflect priority in PSW */
    psw &= ~PSW_PRIO_MASK;
    psw |= priority << PSW_PRIO_SHFT;
    /* save interrupt return address and start service routine */
    WR(30, pc);
    if (V == 0) {
      /* exceptions and interrupts are vectored to ROM */
      pc = 0xC0000000 | ROM_BASE;
    } else {
      /* exceptions and interrupts are vectored to RAM */
      pc = 0xC0000000;
    }
    if (priority == EXC_TLB_MISS &&
        (mmuGetBadAddr() & 0x80000000) == 0) {
      /* user TLB miss exception */
      pc |= 0x00000008;
    } else {
      /* any other exception or interrupt */
      pc |= 0x00000004;
    }
    traceHandleIS(total,RR(30),pc,priority);
  }
}


static void execNextInstruction(void) {
  Word instr;
  Word next;
  int op, reg1, reg2, reg3;
  Half immed;
  Word offset;
  int scnt;
  Word smsk;
  Word aux;

  /* count the instruction */
  total++;
  /* fetch the instruction */
  instr = mmuReadWord(pc, UM);
  /* decode the instruction */
  op = (instr >> 26) & 0x3F;
  reg1 = (instr >> 21) & 0x1F;
  reg2 = (instr >> 16) & 0x1F;
  reg3 = (instr >> 11) & 0x1F;
  immed = instr & 0x0000FFFF;
  offset = instr & 0x03FFFFFF;
  next = pc + 4;
  /* execute the instruction */
  switch (op) {
    case OP_ADD:
      WR(reg3, (signed int) RR(reg1) + (signed int) RR(reg2));
      break;
    case OP_ADDI:
      WR(reg2, (signed int) RR(reg1) + (signed int) SEXT16(immed));
      break;
    case OP_SUB:
      WR(reg3, (signed int) RR(reg1) - (signed int) RR(reg2));
      break;
    case OP_SUBI:
      WR(reg2, (signed int) RR(reg1) - (signed int) SEXT16(immed));
      break;
    case OP_MUL:
      WR(reg3, (signed int) RR(reg1) * (signed int) RR(reg2));
      break;
    case OP_MULI:
      WR(reg2, (signed int) RR(reg1) * (signed int) SEXT16(immed));
      break;
    case OP_MULU:
      WR(reg3, RR(reg1) * RR(reg2));
      break;
    case OP_MULUI:
      WR(reg2, RR(reg1) * ZEXT16(immed));
      break;
    case OP_DIV:
      if (RR(reg2) == 0) {
        throwException(EXC_DIVIDE);
      }
      WR(reg3, (signed int) RR(reg1) / (signed int) RR(reg2));
      break;
    case OP_DIVI:
      if (SEXT16(immed) == 0) {
        throwException(EXC_DIVIDE);
      }
      WR(reg2, (signed int) RR(reg1) / (signed int) SEXT16(immed));
      break;
    case OP_DIVU:
      if (RR(reg2) == 0) {
        throwException(EXC_DIVIDE);
      }
      WR(reg3, RR(reg1) / RR(reg2));
      break;
    case OP_DIVUI:
      if (SEXT16(immed) == 0) {
        throwException(EXC_DIVIDE);
      }
      WR(reg2, RR(reg1) / ZEXT16(immed));
      break;
    case OP_REM:
      if (RR(reg2) == 0) {
        throwException(EXC_DIVIDE);
      }
      WR(reg3, (signed int) RR(reg1) % (signed int) RR(reg2));
      break;
    case OP_REMI:
      if (SEXT16(immed) == 0) {
        throwException(EXC_DIVIDE);
      }
      WR(reg2, (signed int) RR(reg1) % (signed int) SEXT16(immed));
      break;
    case OP_REMU:
      if (RR(reg2) == 0) {
        throwException(EXC_DIVIDE);
      }
      WR(reg3, RR(reg1) % RR(reg2));
      break;
    case OP_REMUI:
      if (SEXT16(immed) == 0) {
        throwException(EXC_DIVIDE);
      }
      WR(reg2, RR(reg1) % ZEXT16(immed));
      break;
    case OP_AND:
      WR(reg3, RR(reg1) & RR(reg2));
      break;
    case OP_ANDI:
      WR(reg2, RR(reg1) & ZEXT16(immed));
      break;
    case OP_OR:
      WR(reg3, RR(reg1) | RR(reg2));
      break;
    case OP_ORI:
      WR(reg2, RR(reg1) | ZEXT16(immed));
      break;
    case OP_XOR:
      WR(reg3, RR(reg1) ^ RR(reg2));
      break;
    case OP_XORI:
      WR(reg2, RR(reg1) ^ ZEXT16(immed));
      break;
    case OP_XNOR:
      WR(reg3, ~(RR(reg1) ^ RR(reg2)));
      break;
    case OP_XNORI:
      WR(reg2, ~(RR(reg1) ^ ZEXT16(immed)));
      break;
    case OP_SLL:
      scnt = RR(reg2) & 0x1F;
      WR(reg3, RR(reg1) << scnt);
      break;
    case OP_SLLI:
      scnt = immed & 0x1F;
      WR(reg2, RR(reg1) << scnt);
      break;
    case OP_SLR:
      scnt = RR(reg2) & 0x1F;
      WR(reg3, RR(reg1) >> scnt);
      break;
    case OP_SLRI:
      scnt = immed & 0x1F;
      WR(reg2, RR(reg1) >> scnt);
      break;
    case OP_SAR:
      scnt = RR(reg2) & 0x1F;
      smsk = (RR(reg1) & 0x80000000 ? ~(((Word) 0xFFFFFFFF) >> scnt) : 0);
      WR(reg3, smsk | (RR(reg1) >> scnt));
      break;
    case OP_SARI:
      scnt = immed & 0x1F;
      smsk = (RR(reg1) & 0x80000000 ? ~(((Word) 0xFFFFFFFF) >> scnt) : 0);
      WR(reg2, smsk | (RR(reg1) >> scnt));
      break;
    case OP_LDHI:
      WR(reg2, ZEXT16(immed) << 16);
      break;
    case OP_BEQ:
      if (RR(reg1) == RR(reg2)) {
        next += SEXT16(immed) << 2;
      }
      break;
    case OP_BNE:
      if (RR(reg1) != RR(reg2)) {
        next += SEXT16(immed) << 2;
      }
      break;
    case OP_BLE:
      if ((signed int) RR(reg1) <= (signed int) RR(reg2)) {
        next += SEXT16(immed) << 2;
      }
      break;
    case OP_BLEU:
      if (RR(reg1) <= RR(reg2)) {
        next += SEXT16(immed) << 2;
      }
      break;
    case OP_BLT:
      if ((signed int) RR(reg1) < (signed int) RR(reg2)) {
        next += SEXT16(immed) << 2;
      }
      break;
    case OP_BLTU:
      if (RR(reg1) < RR(reg2)) {
        next += SEXT16(immed) << 2;
      }
      break;
    case OP_BGE:
      if ((signed int) RR(reg1) >= (signed int) RR(reg2)) {
        next += SEXT16(immed) << 2;
      }
      break;
    case OP_BGEU:
      if (RR(reg1) >= RR(reg2)) {
        next += SEXT16(immed) << 2;
      }
      break;
    case OP_BGT:
      if ((signed int) RR(reg1) > (signed int) RR(reg2)) {
        next += SEXT16(immed) << 2;
      }
      break;
    case OP_BGTU:
      if (RR(reg1) > RR(reg2)) {
        next += SEXT16(immed) << 2;
      }
      break;
    case OP_J:
      next += SEXT26(offset) << 2;
      break;
    case OP_JR:
      traceHandleJR(total,pc,reg1);
      next = RR(reg1);
      break;
    case OP_JAL:
      traceHandleJAL(total,pc,next + (SEXT26(offset) << 2));
      WR(31, next);
      next += SEXT26(offset) << 2;
      break;
    case OP_JALR:
      aux = RR(reg1);
      traceHandleJAL(total,pc,aux);
      WR(31, next);
      next = aux;
      break;
    case OP_TRAP:
      throwException(EXC_TRAP);
      break;
    case OP_RFX:
      if (UM != 0) {
        throwException(EXC_PRV_INSTRCT);
      }
      if (PIE != 0) {
        psw |= PSW_IE;
      } else {
        psw &= ~PSW_IE;
      }
      if (OIE != 0) {
        psw |= PSW_PIE;
      } else {
        psw &= ~PSW_PIE;
      }
      if (PUM != 0) {
        psw |= PSW_UM;
      } else {
        psw &= ~PSW_UM;
      }
      if (OUM != 0) {
        psw |= PSW_PUM;
      } else {
        psw &= ~PSW_PUM;
      }
      traceHandleRFX(total,RR(30));
      next = RR(30);
      break;
    case OP_LDW:
      WR(reg2, mmuReadWord(RR(reg1) + SEXT16(immed), UM));
      break;
    case OP_LDH:
      WR(reg2, (signed int) (signed short)
               mmuReadHalf(RR(reg1) + SEXT16(immed), UM));
      break;
    case OP_LDHU:
      WR(reg2, mmuReadHalf(RR(reg1) + SEXT16(immed), UM));
      break;
    case OP_LDB:
      WR(reg2, (signed int) (signed char)
               mmuReadByte(RR(reg1) + SEXT16(immed), UM));
      break;
    case OP_LDBU:
      WR(reg2, mmuReadByte(RR(reg1) + SEXT16(immed), UM));
      break;
    case OP_STW:
      mmuWriteWord(RR(reg1) + SEXT16(immed), RR(reg2), UM);
      if (isWriteBreakAddr(RR(reg1) + SEXT16(immed), OP_STW))
        cpuHalt();
      break;
    case OP_STH:
      mmuWriteHalf(RR(reg1) + SEXT16(immed), RR(reg2), UM);
      if (isWriteBreakAddr(RR(reg1) + SEXT16(immed), OP_STH))
        cpuHalt();
      break;
    case OP_STB:
      mmuWriteByte(RR(reg1) + SEXT16(immed), RR(reg2), UM);
      if (isWriteBreakAddr(RR(reg1) + SEXT16(immed), OP_STB))
        cpuHalt();
      break;
    case OP_MVFS:
      switch (immed) {
        case 0:
          WR(reg2, psw);
          break;
        case 1:
          WR(reg2, mmuGetIndex());
          break;
        case 2:
          WR(reg2, mmuGetEntryHi());
          break;
        case 3:
          WR(reg2, mmuGetEntryLo());
          break;
        case 4:
          WR(reg2, mmuGetBadAddr());
          break;
        case 5:
          WR(reg2, instrCount >> 32);
          break;
        case 6:
          WR(reg2, instrCount & 0xFFFFFFFF);
          break;
        default:
          throwException(EXC_ILL_INSTRCT);
          break;
      }
      break;
    case OP_MVTS:
      if (UM != 0) {
        throwException(EXC_PRV_INSTRCT);
      }
      switch (immed) {
        case 0:
          psw = RR(reg2);
          break;
        case 1:
          mmuSetIndex(RR(reg2));
          break;
        case 2:
          mmuSetEntryHi(RR(reg2));
          break;
        case 3:
          mmuSetEntryLo(RR(reg2));
          break;
        case 4:
          mmuSetBadAddr(RR(reg2));
          break;
        default:
          throwException(EXC_ILL_INSTRCT);
          break;
      }
      break;
    case OP_TBS:
      if (UM != 0) {
        throwException(EXC_PRV_INSTRCT);
      }
      mmuTbs();
      break;
    case OP_TBWR:
      if (UM != 0) {
        throwException(EXC_PRV_INSTRCT);
      }
      mmuTbwr();
      break;
    case OP_TBRI:
      if (UM != 0) {
        throwException(EXC_PRV_INSTRCT);
      }
      mmuTbri();
      break;
    case OP_TBWI:
      if (UM != 0) {
        throwException(EXC_PRV_INSTRCT);
      }
      mmuTbwi();
      break;
    default:
      throwException(EXC_ILL_INSTRCT);
      break;
  }
  /* update PC */
  pc = next;

  if(stackLevelBreak == traceGetStackLevel()) {
	  /* reset level */
	  stackLevelBreak = -1;
	  /* we want to break after this instruction */
	  stackLevelReached = 1;
  }
}


/**************************************************************/


Word cpuGetPC(void) {
  return pc;
}


void cpuSetPC(Word addr) {
  pc = addr;
}


Word cpuGetReg(int regnum) {
  return RR(regnum & 0x1F);
}


void cpuSetReg(int regnum, Word value) {
  WR(regnum & 0x1F, value);
}


Word cpuGetPSW(void) {
  return psw;
}


void cpuSetPSW(Word value) {
  psw = value;
}


Word cpuGetIRQ(void) {
  return irqPending;
}

int cpuGetStackLevelBreak(void) {
	return stackLevelBreak;
}

void cpuSetStackLevelBreak(int level) {
	stackLevelBreak = level;
}


Bool cpuTestBreak(void) {
  return breakSet;
}


Word cpuGetBreak(void) {
  return breakAddr;
}


void cpuSetBreak(Word addr) {
  breakAddr = addr;
  breakSet = true;
}


void cpuResetBreak(void) {
  breakSet = false;
}


Bool cpuTestWriteBreak(void) {
  return wrBreakSet;
}


Word cpuGetWriteBreak(void) {
  return wrBreakAddr;
}


void cpuSetWriteBreak(Word addr) {
  wrBreakAddr = addr;
  wrBreakSet = true;
}


void cpuResetWriteBreak(void) {
  wrBreakSet = false;
}


Word cpuGetTotal(void) {
  return total;
}


void cpuStep(void) {
  jmp_buf myEnvironment;
  int exception;

  exception = setjmp(myEnvironment);
  if (exception == 0) {
    /* initialization */
    pushEnvironment(&myEnvironment);
    handleRealTimeTasks();
    execNextInstruction();
    handleInterrupts();
  } else {
    /* an exception was thrown */
    cpuSetInterrupt(exception);
    handleInterrupts();
  }
  popEnvironment();
}


void cpuRun(void) {
  jmp_buf myEnvironment;
  int exception;

  run = true;
  exception = setjmp(myEnvironment);
  if (exception == 0) {
    /* initialization */
    pushEnvironment(&myEnvironment);
  } else {
    /* an exception was thrown */
    cpuSetInterrupt(exception);
    handleInterrupts();
    if ((breakSet && pc == breakAddr) || stackLevelReached) {
      run = false;
      stackLevelReached = 0;
    }
  }
  while (run) {
    handleRealTimeTasks();
    execNextInstruction();
    handleInterrupts();
    if ((breakSet && pc == breakAddr) || stackLevelReached) {
      run = false;
      stackLevelReached = 0;
    }
  }
  popEnvironment();
}


void cpuHalt(void) {
  run = false;
}


void cpuSetInterrupt(int priority) {
  irqPending |= ((unsigned) 1 << priority);
}


void cpuResetInterrupt(int priority) {
  irqPending &= ~((unsigned) 1 << priority);
}


void cpuReset(void) {
  int i;

  cPrintf("Resetting CPU...\n");
  /* most registers are in a random state */
  for (i = 1; i < 32; i++) {
    r[i] = rand();
  }
  /* but not all */
  pc = startAddr;
  r[0] = 0;
  psw = 0;
  /* reset simulator control variables */
  instrCount = 0;
  irqPending = 0;
  total = 0;
}


void cpuInit(Word initialPC) {
  startAddr = initialPC;
  cpuReset();
}


void cpuExit(void) {
}
