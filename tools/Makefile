DIRS = $(filter-out $(wildcard *.*) Makefile,$(wildcard *))

.PHONY:  all install clean

all:
		@for i in $(DIRS) ; do \
			$(MAKE) -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

install:
		@for i in $(DIRS) ; do \
			$(MAKE) -C $$i install || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

clean:
		@for i in $(DIRS) ; do \
			$(MAKE) -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
		rm -f *~
