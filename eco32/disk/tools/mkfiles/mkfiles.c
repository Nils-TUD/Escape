/*
 * mkfiles.c -- generate test data files for file system test
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BLOCK_SIZE	4096


int main(int argc, char *argv[]) {
  int j, i, k, m;
  char buf[80];
  unsigned char block[BLOCK_SIZE];
  FILE *dataFile;

  if (argc != 1) {
    printf("Usage: %s\n", argv[0]);
    return 1;
  }
  for (j = 0; j < 12; j++) {
    i = 1 << j;
    sprintf(buf, "0x%06X", i);
    printf("%s\n", buf);
    dataFile = fopen(buf, "wb");
    if (dataFile == NULL) {
      printf("Error: cannot open data file '%s'\n", buf);
      return 1;
    }
    for (m = 0; m < i; m++) {
      for (k = 0; k < BLOCK_SIZE; k += 4) {
        block[k + 0] = m >> 24;
        block[k + 1] = m >> 16;
        block[k + 2] = m >>  8;
        block[k + 3] = m >>  0;
      }
      if (fwrite(block, 1, BLOCK_SIZE, dataFile) != BLOCK_SIZE) {
        printf("Error: cannot write data file '%s'\n", buf);
        return 1;
      }
    }
    fclose(dataFile);
  }
  return 0;
}
