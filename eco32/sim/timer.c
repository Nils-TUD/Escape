/*
 * timer.c -- timer simulation
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "common.h"
#include "console.h"
#include "error.h"
#include "except.h"
#include "cpu.h"
#include "timer.h"


#define TIME_WRAP	1000000		/* avoid overflow of current time */


typedef struct timer {
  struct timer *next;
  int alarm;
  void (*callback)(int param);
  int param;
} Timer;


static Bool debug = false;

static Timer *activeTimers = NULL;
static Timer *freeTimers = NULL;

static int currentTime = 0;

static Word timerCtrl = 0x00000000;
static Word timerDivisor = 0xFFFFFFFF;
static Word timerCounter = 0xFFFFFFFF;


Word timerRead(Word addr) {
  Word data;

  if (debug) {
    cPrintf("\n**** TIMER READ from 0x%08X", addr);
  }
  if (addr == TIMER_CTRL) {
    data = timerCtrl;
  } else
  if (addr == TIMER_DIVISOR) {
    data = timerDivisor;
  } else {
    /* illegal register */
    throwException(EXC_BUS_TIMEOUT);
  }
  if (debug) {
    cPrintf(", data = 0x%08X ****\n", data);
  }
  return data;
}


void timerWrite(Word addr, Word data) {
  if (debug) {
    cPrintf("\n**** TIMER WRITE to 0x%08X, data = 0x%08X ****\n",
            addr, data);
  }
  if (addr == TIMER_CTRL) {
    if (data & TIMER_IEN) {
      timerCtrl |= TIMER_IEN;
    } else {
      timerCtrl &= ~TIMER_IEN;
    }
    if (data & TIMER_EXP) {
      timerCtrl |= TIMER_EXP;
    } else {
      timerCtrl &= ~TIMER_EXP;
    }
    if ((timerCtrl & TIMER_IEN) != 0 &&
        (timerCtrl & TIMER_EXP) != 0) {
      /* raise timer interrupt */
      cpuSetInterrupt(IRQ_TIMER);
    } else {
      /* lower timer interrupt */
      cpuResetInterrupt(IRQ_TIMER);
    }
  } else
  if (addr == TIMER_DIVISOR) {
    timerDivisor = data;
    timerCounter = data;
  } else {
    /* illegal register */
    throwException(EXC_BUS_TIMEOUT);
  }
}


void timerTick(void) {
  Timer *timer;
  void (*callback)(int param);
  int param;

  /* increment current time, avoid overflow */
  if (++currentTime == TIME_WRAP) {
    currentTime -= TIME_WRAP;
    timer = activeTimers;
    while (timer != NULL) {
      timer->alarm -= TIME_WRAP;
      timer = timer->next;
    }
  }
  /* check whether any simulation timer expired */
  while (activeTimers != NULL &&
         currentTime >= activeTimers->alarm) {
    timer = activeTimers;
    activeTimers = timer->next;
    callback = timer->callback;
    param = timer->param;
    timer->next = freeTimers;
    freeTimers = timer;
    (*callback)(param);
  }
  /* decrement counter and check if an interrupt must be raised */
  if (--timerCounter == 0) {
    timerCounter = timerDivisor;
    timerCtrl |= TIMER_EXP;
    if (timerCtrl & TIMER_IEN) {
      /* raise timer interrupt */
      cpuSetInterrupt(IRQ_TIMER);
    }
  }
}


void timerStart(int msec, void (*callback)(int param), int param) {
  Timer *timer;
  Timer *p;

  if (freeTimers == NULL) {
    error("out of timers");
  }
  timer = freeTimers;
  freeTimers = timer->next;
  timer->alarm = currentTime + msec;
  timer->callback = callback;
  timer->param = param;
  if (activeTimers == NULL ||
      timer->alarm < activeTimers->alarm) {
    /* link into front of active timers queue */
    timer->next = activeTimers;
    activeTimers = timer;
  } else {
    /* link elsewhere into active timers queue */
    p = activeTimers;
    while (p->next != NULL &&
           p->next->alarm <= timer->alarm) {
      p = p->next;
    }
    timer->next = p->next;
    p->next = timer;
  }
}


void timerReset(void) {
  Timer *timer;

  cPrintf("Resetting Timer...\n");
  while (activeTimers != NULL) {
    timer = activeTimers;
    activeTimers = timer->next;
    timer->next = freeTimers;
    freeTimers = timer;
  }
}


void timerInit(void) {
  Timer *timer;
  int i;

  for (i = 0; i < NUMBER_TIMERS; i++) {
    timer = malloc(sizeof(Timer));
    if (timer == NULL) {
      error("cannot allocate simulation timers");
    }
    timer->next = freeTimers;
    freeTimers = timer;
  }
  timerReset();
}


void timerExit(void) {
  Timer *timer;

  timerReset();
  while (freeTimers != NULL) {
    timer = freeTimers;
    freeTimers = timer->next;
    free(timer);
  }
}
