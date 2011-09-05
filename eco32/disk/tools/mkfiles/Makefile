#
# Makefile to generate test data files for file system test
#

BUILD = ../../../build

CC = gcc
CFLAGS = -Wall -g

.PHONY:		all install clean

all:		0x000001

install:	0x000001
		mkdir -p $(BUILD)/data
		cp 0x* $(BUILD)/data

0x000001:	mkfiles
		./mkfiles

mkfiles:	mkfiles.c
		$(CC) $(CFLAGS) -o mkfiles mkfiles.c

clean:
		rm -f *~ mkfiles 0x*
