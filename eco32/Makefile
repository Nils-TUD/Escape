#
# Makefile for ECO32 project
#

VERSION = 0.20

DIRS = doc asld sim simtest hwtests monitor disk stdalone
BUILD = `pwd`/build

.PHONY:		all compiler builddir clean dist

all:
		for i in $(DIRS) ; do \
		  $(MAKE) -C $$i install ; \
		done

compiler:	builddir
		$(MAKE) -C lcc BUILDDIR=$(BUILD)/bin \
		               HOSTFILE=etc/eco32-linux.c lcc
		$(MAKE) -C lcc BUILDDIR=$(BUILD)/bin all
		rm -f $(BUILD)/bin/*.c
		rm -f $(BUILD)/bin/*.o
		rm -f $(BUILD)/bin/*.a

builddir:
		mkdir -p $(BUILD)
		mkdir -p $(BUILD)/bin

clean:
		for i in $(DIRS) ; do \
		  $(MAKE) -C $$i clean ; \
		done
		rm -rf $(BUILD)
		rm -f *~

