/*
 * main.c -- the main program
 */


#include "common.h"
#include "stdarg.h"
#include "romlib.h"
#include "command.h"
#include "getline.h"
#include "instr.h"
#include "cpu.h"


int main(void) {
  char *line;

  printf("\n\nECO32 Machine Monitor 1.0\n\n");
  initInstrTable();
  cpuSetPC(0xC0000000);
  
  boot(0);
  
  while (1) {
    line = getLine("ECO32 > ");
    addHist(line);
    execCommand(line);
  }
  return 0;
}
