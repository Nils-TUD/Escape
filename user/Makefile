# TODO
DIRS = $(filter-out $(wildcard *.*) cpp d Makefile,$(wildcard *))

.PHONY:  all clean

all:
		@for i in $(DIRS) ; do \
			$(MAKE) $(JOBS) -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

clean:
		@for i in $(DIRS) ; do \
			$(MAKE) $(JOBS) -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
		rm -f *~
