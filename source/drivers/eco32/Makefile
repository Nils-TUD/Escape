DIRS = $(filter-out Makefile,$(wildcard *))

.PHONY:  all clean

all:
	@for i in $(DIRS) ; do \
		$(MAKE) -s --no-print-directory -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
	done;

clean:
	@for i in $(DIRS) ; do \
		$(MAKE) -s --no-print-directory -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
	done
