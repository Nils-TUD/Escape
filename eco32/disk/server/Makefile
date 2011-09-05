#
# Makefile for serial line disk server
#

BUILD = ../../build

.PHONY:		all install clean

all:		diskserv

install:	diskserv
		mkdir -p $(BUILD)/bin
		cp diskserv $(BUILD)/bin

diskserv:	diskserv.c
		gcc -Wall -o diskserv diskserv.c

clean:
		rm -f *~ diskserv
