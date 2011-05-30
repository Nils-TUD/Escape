ifeq ($(ARCH),eco32)
	DIRS = $(filter-out $(wildcard *.*) cpp d Makefile,$(wildcard *))
else
	DIRS = $(filter-out $(wildcard *.*) d Makefile,$(wildcard *))
endif

.PHONY:  all clean

all:
		@for i in $(DIRS) ; do \
			$(MAKE) -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

clean:
		@for i in $(DIRS) ; do \
			$(MAKE) -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
		rm -f *~
