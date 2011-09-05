/*
 * bin2exo.c -- convert binary data to Motorola S-records
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


void error(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  printf("Error: ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
  exit(1);
}


int main(int argc, char *argv[]) {
  char *endptr;
  unsigned int loadAddr;
  FILE *infile;
  FILE *outfile;
  int numBytes, i;
  int c;
  unsigned char lineData[16];
  unsigned int chksum;

  if (argc != 4) {
    printf("Usage: %s <load addr, hex> <input file> <output file>\n",
           argv[0]);
    exit(1);
  }
  loadAddr = strtoul(argv[1], &endptr, 16);
  if (*endptr != '\0') {
    error("illegal load address %s", argv[1]);
  }
  infile = fopen(argv[2], "rb");
  if (infile == NULL) {
    error("cannot open input file %s", argv[2]);
  }
  outfile = fopen(argv[3], "wt");
  if (outfile == NULL) {
    error("cannot open output file %s", argv[3]);
  }
  while (1) {
    chksum = 0;
    for (numBytes = 0; numBytes < 16; numBytes++) {
      c = fgetc(infile);
      if (c == EOF) {
        break;
      }
      lineData[numBytes] = c;
      chksum += c;
    }
    if (numBytes == 0) {
      break;
    }
    fprintf(outfile, "S2%02X%06X", numBytes + 4, loadAddr);
    for (i = 0; i < numBytes; i++) {
      fprintf(outfile, "%02X", lineData[i]);
    }
    chksum += numBytes + 4;
    chksum += ((loadAddr >>  0) & 0xFF) +
              ((loadAddr >>  8) & 0xFF) +
              ((loadAddr >> 16) & 0xFF);
    fprintf(outfile, "%02X\n", 0xFF - (chksum & 0xFF));
    loadAddr += numBytes;
    if (c == EOF) {
      break;
    }
  }
  fprintf(outfile, "S804000000FB\n");
  fclose(infile);
  fclose(outfile);
  return 0;
}
