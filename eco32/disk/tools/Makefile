#
# Makefile for EOS32 disk tools
#

BUILD = ../../build

DIRS = mkdisk mkmboot mkpart shpart mkboot mkfiles mkfs shfs fsck fspoke

.PHONY:		all install clean

all:
		for i in $(DIRS) ; do \
		  $(MAKE) -C $$i all ; \
		done

install:
		for i in $(DIRS) ; do \
		  $(MAKE) -C $$i install ; \
		done

clean:
		for i in $(DIRS) ; do \
		  $(MAKE) -C $$i clean ; \
		done
		rm -f *~
