BUILD = ../build
DIRS = $(filter-out $(wildcard *.*) Makefile,$(wildcard *))
HDDMOUNT = ../disk
SERVICES = services.txt

.PHONY:  all serv clean

all:	$(SERVICES)
		@for i in $(DIRS) ; do \
			$(MAKE) -s --no-print-directory -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done;

$(SERVICES):
		../tools/disk.sh copy $(SERVICES) /etc/services

clean:
		@for i in $(DIRS) ; do \
			$(MAKE) -s --no-print-directory -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
