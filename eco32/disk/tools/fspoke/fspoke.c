/*
 * fspoke.c -- file system poke utility
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#define SSIZE	512	/* disk sector size in bytes */


void error(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  printf("Error: ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
  exit(1);
}


unsigned int read4FromEco(unsigned char *p) {
  return (unsigned int) p[0] << 24 |
         (unsigned int) p[1] << 16 |
         (unsigned int) p[2] <<  8 |
         (unsigned int) p[3] <<  0;
}


void write4ToEco(unsigned char *p, unsigned int data) {
  p[0] = data >> 24;
  p[1] = data >> 16;
  p[2] = data >>  8;
  p[3] = data >>  0;
}


void write2ToEco(unsigned char *p, unsigned int data) {
  p[0] = data >> 8;
  p[1] = data >> 0;
}


void write1ToEco(unsigned char *p, unsigned int data) {
  p[0] = data >> 0;
}


void usage(char *myself) {
  printf("Usage: %s <disk> <partition> <offset> <width> <value>\n", myself);
  printf("       <disk> is a disk image file name\n");
  printf("       <partition> is a partition number ");
  printf("(or '*' for the whole disk)\n");
  printf("       <offset> is a partition or disk relative byte offset\n");
  printf("       <width> is b (byte), h (halfword), or w (word)\n");
  printf("       <value> is the data to be poked at <offset>\n");
  exit(1);
}


int main(int argc, char *argv[]) {
  FILE *disk;
  unsigned long fsStart;	/* file system start sector */
  unsigned long fsSize;		/* file system size in sectors */
  int part;
  char *endptr;
  unsigned char partTable[SSIZE];
  unsigned char *ptptr;
  unsigned long partType;
  unsigned long offset;
  unsigned int value;
  unsigned char valBuf[4];
  int width;

  if (argc != 6) {
    usage(argv[0]);
  }
  disk = fopen(argv[1], "r+b");
  if (disk == NULL) {
    error("cannot open disk image file '%s'", argv[1]);
  }
  if (strcmp(argv[2], "*") == 0) {
    /* whole disk contains one single file system */
    fsStart = 0;
    fseek(disk, 0, SEEK_END);
    fsSize = ftell(disk) / SSIZE;
  } else {
    /* argv[2] is partition number of file system */
    part = strtoul(argv[2], &endptr, 10);
    if (*endptr != '\0' || part < 0 || part > 15) {
      error("illegal partition number '%s'", argv[2]);
    }
    fseek(disk, 1 * SSIZE, SEEK_SET);
    if (fread(partTable, 1, SSIZE, disk) != SSIZE) {
      error("cannot read partition table of disk '%s'", argv[1]);
    }
    ptptr = partTable + part * 32;
    partType = read4FromEco(ptptr + 0);
    if ((partType & 0x7FFFFFFF) != 0x00000058) {
      error("partition %d of disk '%s' does not contain an EOS32 file system",
            part, argv[1]);
    }
    fsStart = read4FromEco(ptptr + 4);
    fsSize = read4FromEco(ptptr + 8);
  }
  offset = strtoul(argv[3], &endptr, 0);
  if (*endptr != '\0') {
    error("illegal offset '%s'", argv[3]);
  }
  value = strtoul(argv[5], &endptr, 0);
  if (*endptr != '\0') {
    error("illegal value '%s'", argv[5]);
  }
  if (strcmp(argv[4], "b") == 0) {
    write1ToEco(valBuf, value);
    width = 1;
  } else
  if (strcmp(argv[4], "h") == 0) {
    write2ToEco(valBuf, value);
    width = 2;
  } else
  if (strcmp(argv[4], "w") == 0) {
    write4ToEco(valBuf, value);
    width = 4;
  } else {
    error("illegal width '%s'", argv[4]);
  }
  fseek(disk, fsStart * SSIZE + offset, SEEK_SET);
  if (fwrite(valBuf, 1, width, disk) != width) {
    error("cannot write %s value 0x%02X at offset 0x%08X on image '%s'",
          width == 1 ? "byte" :
          width == 2 ? "halfword" :
          width == 4 ? "word" :
          "unknown width",
          value, offset, argv[1]);
  }
  fclose(disk);
  return 0;
}
