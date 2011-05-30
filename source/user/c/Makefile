ifeq ($(ARCH),eco32)
	DIRS = $(filter-out $(wildcard *.*) libctest shlibtest dynlink x86mark Makefile,$(wildcard *))
else
	DIRS = $(filter-out $(wildcard *.*) shlibtest Makefile,$(wildcard *))
endif

.PHONY:  all clean

all:
		@for i in $(DIRS) ; do \
			$(MAKE) -s --no-print-directory -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

clean:
		@for i in $(DIRS) ; do \
			$(MAKE) -s --no-print-directory -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
		rm -f *~
