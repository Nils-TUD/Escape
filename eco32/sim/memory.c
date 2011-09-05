/*
 * memory.c -- main memory simulation
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
#include "memory.h"
#include "timer.h"
#include "dspkbd.h"
#include "term.h"
#include "disk.h"
#include "output.h"
#include "graph.h"


static Byte *rom;
static Byte *mem;
static FILE *romImage;
static unsigned int romSize;
static FILE *progImage;
static unsigned int progSize;


Word memoryReadWord(Word pAddr) {
  Word data;

  if (pAddr <= MEMORY_SIZE - 4) {
    data = ((Word) *(mem + pAddr + 0)) << 24 |
           ((Word) *(mem + pAddr + 1)) << 16 |
           ((Word) *(mem + pAddr + 2)) <<  8 |
           ((Word) *(mem + pAddr + 3)) <<  0;
    return data;
  }
  if (pAddr >= ROM_BASE &&
      pAddr <= ROM_BASE + ROM_SIZE - 4) {
    data = ((Word) *(rom + (pAddr - ROM_BASE) + 0)) << 24 |
           ((Word) *(rom + (pAddr - ROM_BASE) + 1)) << 16 |
           ((Word) *(rom + (pAddr - ROM_BASE) + 2)) <<  8 |
           ((Word) *(rom + (pAddr - ROM_BASE) + 3)) <<  0;
    return data;
  }
  if ((pAddr & IO_DEV_MASK) == TIMER_BASE) {
    data = timerRead(pAddr & IO_REG_MASK);
    return data;
  }
  if ((pAddr & IO_DEV_MASK) == DISPLAY_BASE) {
    data = displayRead(pAddr & IO_REG_MASK);
    return data;
  }
  if ((pAddr & IO_DEV_MASK) == KEYBOARD_BASE) {
    data = keyboardRead(pAddr & IO_REG_MASK);
    return data;
  }
  if ((pAddr & IO_DEV_MASK) == TERM_BASE) {
    data = termRead(pAddr & IO_REG_MASK);
    return data;
  }
  if ((pAddr & IO_DEV_MASK) == DISK_BASE) {
    data = diskRead(pAddr & IO_REG_MASK);
    return data;
  }
  if ((pAddr & IO_DEV_MASK) == OUTPUT_BASE) {
    data = outputRead(pAddr & IO_REG_MASK);
    return data;
  }
  if ((pAddr & IO_DEV_MASK) >= GRAPH_BASE) {
    data = graphRead(pAddr & IO_GRAPH_MASK);
    return data;
  }
  /* throw bus timeout exception */
  throwException(EXC_BUS_TIMEOUT);
  /* not reached */
  data = 0;
  return data;
}


Half memoryReadHalf(Word pAddr) {
  Half data;

  if (pAddr <= MEMORY_SIZE - 2) {
    data = ((Half) *(mem + pAddr + 0)) << 8 |
           ((Half) *(mem + pAddr + 1)) << 0;
    return data;
  }
  if (pAddr >= ROM_BASE &&
      pAddr <= ROM_BASE + ROM_SIZE - 2) {
    data = ((Half) *(rom + (pAddr - ROM_BASE) + 0)) << 8 |
           ((Half) *(rom + (pAddr - ROM_BASE) + 1)) << 0;
    return data;
  }
  /* throw bus timeout exception */
  throwException(EXC_BUS_TIMEOUT);
  /* not reached */
  data = 0;
  return data;
}


Byte memoryReadByte(Word pAddr) {
  Byte data;

  if (pAddr <= MEMORY_SIZE - 1) {
    data = ((Byte) *(mem + pAddr + 0)) << 0;
    return data;
  }
  if (pAddr >= ROM_BASE &&
      pAddr <= ROM_BASE + ROM_SIZE - 1) {
    data = ((Byte) *(rom + (pAddr - ROM_BASE) + 0)) << 0;
    return data;
  }
  /* throw bus timeout exception */
  throwException(EXC_BUS_TIMEOUT);
  /* not reached */
  data = 0;
  return data;
}


void memoryWriteWord(Word pAddr, Word data) {
  if (pAddr <= MEMORY_SIZE - 4) {
    *(mem + pAddr + 0) = (Byte) (data >> 24);
    *(mem + pAddr + 1) = (Byte) (data >> 16);
    *(mem + pAddr + 2) = (Byte) (data >>  8);
    *(mem + pAddr + 3) = (Byte) (data >>  0);
    return;
  }
  if ((pAddr & IO_DEV_MASK) == TIMER_BASE) {
    timerWrite(pAddr & IO_REG_MASK, data);
    return;
  }
  if ((pAddr & IO_DEV_MASK) == DISPLAY_BASE) {
    displayWrite(pAddr & IO_REG_MASK, data);
    return;
  }
  if ((pAddr & IO_DEV_MASK) == KEYBOARD_BASE) {
    keyboardWrite(pAddr & IO_REG_MASK, data);
    return;
  }
  if ((pAddr & IO_DEV_MASK) == TERM_BASE) {
    termWrite(pAddr & IO_REG_MASK, data);
    return;
  }
  if ((pAddr & IO_DEV_MASK) == DISK_BASE) {
    diskWrite(pAddr & IO_REG_MASK, data);
    return;
  }
  if ((pAddr & IO_DEV_MASK) == OUTPUT_BASE) {
    outputWrite(pAddr & IO_REG_MASK, data);
    return;
  }
  if ((pAddr & IO_DEV_MASK) >= GRAPH_BASE) {
    graphWrite(pAddr & IO_GRAPH_MASK, data);
    return;
  }
  /* throw bus timeout exception */
  throwException(EXC_BUS_TIMEOUT);
}


void memoryWriteHalf(Word pAddr, Half data) {
  if (pAddr <= MEMORY_SIZE - 2) {
    *(mem + pAddr + 0) = (Byte) (data >> 8);
    *(mem + pAddr + 1) = (Byte) (data >> 0);
    return;
  }
  /* throw bus timeout exception */
  throwException(EXC_BUS_TIMEOUT);
}


void memoryWriteByte(Word pAddr, Byte data) {
  if (pAddr <= MEMORY_SIZE - 1) {
    *(mem + pAddr + 0) = (Byte) (data >> 0);
    return;
  }
  /* throw bus timeout exception */
  throwException(EXC_BUS_TIMEOUT);
}


void memoryReset(void) {
  unsigned int i;

  cPrintf("Resetting Memory...\n");
  for (i = 0; i < ROM_SIZE; i++) {
    rom[i] = rand();
  }
  for (i = 0; i < MEMORY_SIZE; i++) {
    mem[i] = rand();
  }
  if (romImage != NULL) {
    fseek(romImage, 0, SEEK_SET);
    if (fread(rom, romSize, 1, romImage) != 1) {
      error("cannot read ROM image file");
    }
    for (i = romSize; i < ROM_SIZE; i++) {
      rom[i] = 0xFF;
    }
    cPrintf("ROM of size %d/%d bytes installed.\n", romSize, ROM_SIZE);
  }
  if (progImage != NULL) {
    fseek(progImage, 0, SEEK_SET);
    if (fread(mem, progSize, 1, progImage) != 1) {
      error("cannot read program image file");
    }
    cPrintf("Program of size %d loaded.\n", progSize);
  }
}


void memoryInit(char *romImageName, char *progImageName) {
  /* allocate ROM */
  rom = malloc(ROM_SIZE);
  if (rom == NULL) {
    error("cannot allocate ROM");
  }
  /* allocate main memory */
  mem = malloc(MEMORY_SIZE);
  if (mem == NULL) {
    error("cannot allocate main memory");
  }
  /* possibly load ROM image */
  if (romImageName == NULL) {
    /* no ROM to plug in */
    romImage = NULL;
  } else {
    /* plug in ROM */
    romImage = fopen(romImageName, "rb");
    if (romImage == NULL) {
      error("cannot open ROM image '%s'", romImageName);
    }
    fseek(romImage, 0, SEEK_END);
    romSize = ftell(romImage);
    if (romSize > ROM_SIZE) {
      error("ROM image too big");
    }
  }
  /* possibly load program image */
  if (progImageName == NULL) {
    /* no program to load */
    progImage = NULL;
  } else {
    /* load program */
    progImage = fopen(progImageName, "rb");
    if (progImage == NULL) {
      error("cannot open program file '%s'", progImageName);
    }
    fseek(progImage, 0, SEEK_END);
    progSize = ftell(progImage);
    if (progSize > MEMORY_SIZE) {
      error("program file too big");
    }
  }
  memoryReset();
}


void memoryExit(void) {
  free(mem);
  free(rom);
}
