/*
 * Disassembler for the a.out-ECO32-format.
 * Build from dof and sim with trace-extension
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "common.h"
#include "instr.h"
#include "map.h"
#include "a.out.h"

static char instrBuffer[100];
static FILE *inFile;
static unsigned int codeStart;
static unsigned int baseAddr;
static ExecHeader execHeader;


static void error(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  printf("Error: ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
  exit(1);
}


static unsigned int read4FromEco(unsigned char *p) {
  return (unsigned int) p[0] << 24 |
         (unsigned int) p[1] << 16 |
         (unsigned int) p[2] <<  8 |
         (unsigned int) p[3] <<  0;
}


static void conv4FromEcoToX86(unsigned char *p) {
  unsigned int data;

  data = read4FromEco(p);
  * (unsigned int *) p = data;
}


static void disasmN(char *opName, Word immed) {
  if (immed == 0) {
    sprintf(instrBuffer, "%-7s", opName);
  } else {
    sprintf(instrBuffer, "%-7s %08X", opName, immed);
  }
}


static void disasmRH(char *opName, int r1, Half immed) {
  sprintf(instrBuffer, "%-7s $%d,%04X", opName, r1, immed);
}


static void disasmRHH(char *opName, int r1, Half immed) {
  sprintf(instrBuffer, "%-7s $%d,%08X", opName, r1, (Word) immed << 16);
}


static void disasmRRH(char *opName, int r1, int r2, Half immed) {
  sprintf(instrBuffer, "%-7s $%d,$%d,%04X", opName, r1, r2, immed);
}


static void disasmRRS(char *opName, int r1, int r2, Half immed) {
  sprintf(instrBuffer, "%-7s $%d,$%d,%s%04X", opName, r1, r2,
          SIGN(16) & immed ? "-" : "+",
          SIGN(16) & immed ? -SEXT16(immed) : SEXT16(immed));
}


static void disasmRRR(char *opName, int r1, int r2, int r3) {
  sprintf(instrBuffer, "%-7s $%d,$%d,$%d", opName, r1, r2, r3);
}


static void disasmRRB(char *opName, int r1, int r2, Half offset, Word locus) {
  sprintf(instrBuffer, "%-7s $%d,$%d,%08X", opName,
          r1, r2, (locus + 4) + (SEXT16(offset) << 2));
}


static void disasmJ(char *opName, Word offset, Word locus) {
  char *funcName = traceGetFuncName(baseAddr + (locus + 4) + (SEXT26(offset) << 2));
  if(funcName != NULL) {
    sprintf(instrBuffer, "%-7s %08X <%s>", opName,
    		baseAddr + (locus + 4) + (SEXT26(offset) << 2),funcName);
  }
  else {
    sprintf(instrBuffer, "%-7s %08X", opName,
    		baseAddr + (locus + 4) + (SEXT26(offset) << 2));
  }
}


static void disasmJR(char *opName, int r1) {
  sprintf(instrBuffer, "%-7s $%d", opName, r1);
}


static char *disasm(Word instr, Word locus) {
  Byte opcode;
  Instr *ip;

  opcode = (instr >> 26) & 0x3F;
  ip = instrCodeTbl[opcode];
  if (ip == NULL) {
    disasmN("???", 0);
  } else {
    switch (ip->format) {
      case FORMAT_N:
        disasmN(ip->name, instr & 0x03FFFFFF);
        break;
      case FORMAT_RH:
        disasmRH(ip->name, (instr >> 16) & 0x1F, instr & 0x0000FFFF);
        break;
      case FORMAT_RHH:
        disasmRHH(ip->name, (instr >> 16) & 0x1F, instr & 0x0000FFFF);
        break;
      case FORMAT_RRH:
        disasmRRH(ip->name, (instr >> 16) & 0x1F,
                  (instr >> 21) & 0x1F, instr & 0x0000FFFF);
        break;
      case FORMAT_RRS:
        disasmRRS(ip->name, (instr >> 16) & 0x1F,
                  (instr >> 21) & 0x1F, instr & 0x0000FFFF);
        break;
      case FORMAT_RRR:
        disasmRRR(ip->name, (instr >> 11) & 0x1F,
                  (instr >> 21) & 0x1F, (instr >> 16) & 0x1F);
        break;
      case FORMAT_RRX:
        if ((opcode & 1) == 0) {
          /* the FORMAT_RRR variant */
          disasmRRR(ip->name, (instr >> 11) & 0x1F,
                    (instr >> 21) & 0x1F, (instr >> 16) & 0x1F);
        } else {
          /* the FORMAT_RRH variant */
          disasmRRH(ip->name, (instr >> 16) & 0x1F,
                    (instr >> 21) & 0x1F, instr & 0x0000FFFF);
        }
        break;
      case FORMAT_RRY:
        if ((opcode & 1) == 0) {
          /* the FORMAT_RRR variant */
          disasmRRR(ip->name, (instr >> 11) & 0x1F,
                    (instr >> 21) & 0x1F, (instr >> 16) & 0x1F);
        } else {
          /* the FORMAT_RRS variant */
          disasmRRS(ip->name, (instr >> 16) & 0x1F,
                    (instr >> 21) & 0x1F, instr & 0x0000FFFF);
        }
        break;
      case FORMAT_RRB:
        disasmRRB(ip->name, (instr >> 21) & 0x1F,
                  (instr >> 16) & 0x1F, instr & 0x0000FFFF, locus);
        break;
      case FORMAT_J:
        disasmJ(ip->name, instr & 0x03FFFFFF, locus);
        break;
      case FORMAT_JR:
        disasmJR(ip->name, (instr >> 21) & 0x1F);
        break;
      default:
      	error("illegal entry in instruction table");
        break;
    }
  }
  return instrBuffer;
}

static void readHeader(void) {
  if(fseek(inFile, 0, SEEK_SET) < 0)
    error("cannot seek to exec header");
  if(fread(&execHeader, sizeof(ExecHeader), 1, inFile) != 1)
    error("cannot read exec header");
  conv4FromEcoToX86((unsigned char *) &execHeader.magic);
  conv4FromEcoToX86((unsigned char *) &execHeader.csize);
  conv4FromEcoToX86((unsigned char *) &execHeader.dsize);
  conv4FromEcoToX86((unsigned char *) &execHeader.bsize);
  conv4FromEcoToX86((unsigned char *) &execHeader.crsize);
  conv4FromEcoToX86((unsigned char *) &execHeader.drsize);
  conv4FromEcoToX86((unsigned char *) &execHeader.symsize);
  conv4FromEcoToX86((unsigned char *) &execHeader.strsize);
}

static void disasmCode(void) {
	unsigned int pos;
	char *funcName;
	char *line;
	Word instr;
	if(fseek(inFile,codeStart,SEEK_SET) < 0)
		error("cannot seek to code-segment");

	pos = 0;
	while(pos < execHeader.csize && fread(&instr,sizeof(Word),1,inFile) == 1) {
		conv4FromEcoToX86((unsigned char*)&instr);
		line = disasm(instr,pos);
		funcName = traceGetFuncName(baseAddr + pos);
		if(funcName != NULL)
			printf("\n%08x <%s>:\n",baseAddr + pos,funcName);
		printf("%08x:       %08x  %s\n",baseAddr + pos,instr,line);
		pos += 4;
	}
}

int main(int argc,char *argv[]) {
	const char *inName;
	if(argc < 2)
		error("Usage: %s <file> [<map>]\n",argv[0]);

	inName = argv[1];
	inFile = fopen(inName, "r");
	if(inFile == NULL)
		error("cannot open input file '%s'",inName);

	if(argc > 2)
		baseAddr = traceParseMap(argv[2]);
	initInstrTable();
	readHeader();

	printf("\n");
	if(execHeader.magic != EXEC_MAGIC) {
		/* if the magic-number is wrong, we assume that the binary has no header */
		if(fseek(inFile,0,SEEK_END) < 0)
			error("cannot seek to end");
		execHeader.csize = ftell(inFile);
		printf("%s:     file format not recognized\n",inName);
		printf("%*s      assuming ECO32-binary without header @ 0xC0000000\n",strlen(inName)," ");
		codeStart = 0;
		baseAddr = 0xC0000000;
	}
	else {
		codeStart = sizeof(ExecHeader);
		printf("%s:     file format a.out-ECO32\n",inName);
		printf("\n");
	}

	printf("\n");
	printf("Disassembly of section code:\n");
	printf("\n");

	disasmCode();

	fclose(inFile);

	return EXIT_SUCCESS;
}

