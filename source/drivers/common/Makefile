ifneq ($(ARCH),i586)
	DIRS = $(filter-out powermng Makefile,$(wildcard *))
else
	DIRS = $(filter-out Makefile,$(wildcard *))
endif

.PHONY:  all clean

all:
		@for i in $(DIRS) ; do \
			$(MAKE) -s --no-print-directory -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done;

clean:
		@for i in $(DIRS) ; do \
			$(MAKE) -s --no-print-directory -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
