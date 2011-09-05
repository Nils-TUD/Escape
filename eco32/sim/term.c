/*
 * term.c -- terminal simulation
 */


#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "common.h"
#include "console.h"
#include "error.h"
#include "except.h"
#include "cpu.h"
#include "timer.h"
#include "term.h"


/**************************************************************/


static Bool debug = false;


typedef struct {
  pid_t pid;
  FILE *in;
  FILE *out;
  Word rcvrCtrl;
  Word rcvrData;
  int rcvrIRQ;
  Word xmtrCtrl;
  Word xmtrData;
  int xmtrIRQ;
} Terminal;


static Terminal terminals[MAX_NTERMS];
static int numTerminals;


/**************************************************************/


static void rcvrCallback(int dev) {
  int c;

  if (debug) {
    cPrintf("\n**** TERM RCVR CALLBACK ****\n");
  }
  timerStart(TERM_RCVR_MSEC, rcvrCallback, dev);
  c = fgetc(terminals[dev].in);
  if (c == EOF) {
    /* no character typed */
    return;
  }
  /* any character typed */
  terminals[dev].rcvrData = c & 0xFF;
  terminals[dev].rcvrCtrl |= TERM_RCVR_RDY;
  if (terminals[dev].rcvrCtrl & TERM_RCVR_IEN) {
    /* raise terminal rcvr interrupt */
    cpuSetInterrupt(terminals[dev].rcvrIRQ);
  }
}


static void xmtrCallback(int dev) {
  if (debug) {
    cPrintf("\n**** TERM XMTR CALLBACK ****\n");
  }
  fputc(terminals[dev].xmtrData & 0xFF, terminals[dev].out);
  terminals[dev].xmtrCtrl |= TERM_XMTR_RDY;
  if (terminals[dev].xmtrCtrl & TERM_XMTR_IEN) {
    /* raise terminal xmtr interrupt */
    cpuSetInterrupt(terminals[dev].xmtrIRQ);
  }
}


/**************************************************************/


Word termRead(Word addr) {
  int dev, reg;
  Word data;

  if (debug) {
    cPrintf("\n**** TERM READ from 0x%08X, reg=0x%x, dev=0x%x", addr, addr & 0x0F, addr >> 4);
  }
  dev = addr >> 4;
  if (dev >= numTerminals) {
    /* illegal device */
    throwException(EXC_BUS_TIMEOUT);
  }
  reg = addr & 0x0F;
  if (reg == TERM_RCVR_CTRL) {
    data = terminals[dev].rcvrCtrl;
  } else
  if (reg == TERM_RCVR_DATA) {
    terminals[dev].rcvrCtrl &= ~TERM_RCVR_RDY;
    if (terminals[dev].rcvrCtrl & TERM_RCVR_IEN) {
      /* lower terminal rcvr interrupt */
      cpuResetInterrupt(terminals[dev].rcvrIRQ);
    }
    data = terminals[dev].rcvrData;
  } else
  if (reg == TERM_XMTR_CTRL) {
    data = terminals[dev].xmtrCtrl;
  } else
  if (reg == TERM_XMTR_DATA) {
    /* this register is write-only */
    throwException(EXC_BUS_TIMEOUT);
  } else {
    /* illegal register */
    throwException(EXC_BUS_TIMEOUT);
  }
  if (debug) {
    cPrintf(", data = 0x%08X ****\n", data);
  }
  return data;
}


void termWrite(Word addr, Word data) {
  int dev, reg;

  if (debug) {
    cPrintf("\n**** TERM WRITE to 0x%08X, data = 0x%08X ****\n",
            addr, data);
  }
  dev = addr >> 4;
  if (dev >= numTerminals) {
    /* illegal device */
    throwException(EXC_BUS_TIMEOUT);
  }
  reg = addr & 0x0F;
  if (reg == TERM_RCVR_CTRL) {
    if (data & TERM_RCVR_IEN) {
      terminals[dev].rcvrCtrl |= TERM_RCVR_IEN;
    } else {
      terminals[dev].rcvrCtrl &= ~TERM_RCVR_IEN;
    }
    if (data & TERM_RCVR_RDY) {
      terminals[dev].rcvrCtrl |= TERM_RCVR_RDY;
    } else {
      terminals[dev].rcvrCtrl &= ~TERM_RCVR_RDY;
    }
    if ((terminals[dev].rcvrCtrl & TERM_RCVR_IEN) != 0 &&
        (terminals[dev].rcvrCtrl & TERM_RCVR_RDY) != 0) {
      /* raise terminal rcvr interrupt */
      cpuSetInterrupt(terminals[dev].rcvrIRQ);
    } else {
      /* lower terminal rcvr interrupt */
      cpuResetInterrupt(terminals[dev].rcvrIRQ);
    }
  } else
  if (reg == TERM_RCVR_DATA) {
    /* this register is read-only */
    throwException(EXC_BUS_TIMEOUT);
  } else
  if (reg == TERM_XMTR_CTRL) {
    if (data & TERM_XMTR_IEN) {
      terminals[dev].xmtrCtrl |= TERM_XMTR_IEN;
    } else {
      terminals[dev].xmtrCtrl &= ~TERM_XMTR_IEN;
    }
    if (data & TERM_XMTR_RDY) {
      terminals[dev].xmtrCtrl |= TERM_XMTR_RDY;
    } else {
      terminals[dev].xmtrCtrl &= ~TERM_XMTR_RDY;
    }
    if ((terminals[dev].xmtrCtrl & TERM_XMTR_IEN) != 0 &&
        (terminals[dev].xmtrCtrl & TERM_XMTR_RDY) != 0) {
      /* raise terminal xmtr interrupt */
      cpuSetInterrupt(terminals[dev].xmtrIRQ);
    } else {
      /* lower terminal xmtr interrupt */
      cpuResetInterrupt(terminals[dev].xmtrIRQ);
    }
  } else
  if (reg == TERM_XMTR_DATA) {
    terminals[dev].xmtrData = data & 0xFF;
    terminals[dev].xmtrCtrl &= ~TERM_XMTR_RDY;
    if (terminals[dev].xmtrCtrl & TERM_XMTR_IEN) {
      /* lower terminal xmtr interrupt */
      cpuResetInterrupt(terminals[dev].xmtrIRQ);
    }
    timerStart(TERM_XMTR_MSEC, xmtrCallback, dev);
  } else {
    /* illegal register */
    throwException(EXC_BUS_TIMEOUT);
  }
}


/**************************************************************/


void termReset(void) {
  int i;

  cPrintf("Resetting Terminals...\n");
  for (i = 0; i < numTerminals; i++) {
    terminals[i].rcvrCtrl = 0;
    terminals[i].rcvrData = 0;
    terminals[i].rcvrIRQ = IRQ_TERM_0_RCVR + 2 * i;
    timerStart(TERM_RCVR_MSEC, rcvrCallback, i);
    terminals[i].xmtrCtrl = TERM_XMTR_RDY;
    terminals[i].xmtrData = 0;
    terminals[i].xmtrIRQ = IRQ_TERM_0_XMTR + 2 * i;
  }
}


static int openPty(int *master, int *slave, char *name) {
  /* try to open master */
  strcpy(name, "/dev/ptmx");
  *master = open(name, O_RDWR | O_NONBLOCK);
  if (*master < 0) {
    /* open failed */
    return -1;
  }
  grantpt(*master);
  unlockpt(*master);
  /* master opened, try to open slave */
  strcpy(name, ptsname(*master));
  *slave = open(name, O_RDWR | O_NONBLOCK);
  if (*slave < 0) {
    /* open failed, close master */
    close(*master);
    return -1;
  }
  /* all is well */
  return 0;
}


static void makeRaw(struct termios *tp) {
  tp->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
  tp->c_oflag &= ~OPOST;
  tp->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
  tp->c_cflag &= ~(CSIZE|PARENB);
  tp->c_cflag |= CS8;
}


void termInit(int numTerms) {
  int master, slave;
  char ptyName[100];
  char ptyTitle[100];
  struct termios termios;
  int i;

  numTerminals = numTerms;
  for (i = 0; i < numTerminals; i++) {
    /* open pseudo terminal */
    if (openPty(&master, &slave, ptyName) < 0) {
      error("cannot open pseudo terminal %d", i);
    }
    if (debug) {
      cPrintf("pseudo terminal '%s': master fd = %d, slave fd = %d\n",
              ptyName, master, slave);
    }
    /* set mode to raw */
    tcgetattr(slave, &termios);
    makeRaw(&termios);
    tcsetattr(slave, TCSANOW, &termios);
    /* fork and exec a new xterm */
    terminals[i].pid = fork();
    if (terminals[i].pid < 0) {
      error("cannot fork xterm process %d", i);
    }
    if (terminals[i].pid == 0) {
      char geo[20];
      /* terminal process */
      setpgid(0, 0);
      close(2);
      close(master);
      /* it's annoying to have to adjust the window-position every time eco-sim starts :) */
      sprintf(geo, "+%d+%d", 1600 + (i % 2) * 500,(i / 2) * 400);
      sprintf(ptyName, "-Sab%d", slave);
      sprintf(ptyTitle, "ECO32 Terminal %d", i);
      execlp("xterm", "xterm", "-geo", geo, "-title", ptyTitle, ptyName, NULL);
      error("cannot exec xterm process %d", i);
    }
    terminals[i].in = fdopen(master, "r");
    setvbuf(terminals[i].in, NULL, _IONBF, 0);
    terminals[i].out = fdopen(master, "w");
    setvbuf(terminals[i].out, NULL, _IONBF, 0);
    /* skip the window id written by xterm */
    while (fgetc(terminals[i].in) != '\n') ;
  }
  termReset();
}


void termExit(void) {
  int i;

  /* kill and wait for all xterm processes */
  for (i = 0; i < numTerminals; i++) {
    if (terminals[i].pid > 0) {
      kill(terminals[i].pid, SIGKILL);
      waitpid(terminals[i].pid, NULL, 0);
    }
  }
}
