BUILD = ../build
DIRS = $(filter-out $(wildcard *.*) Makefile,$(wildcard *))
HDDMOUNT = ../disk
SERVICES = services.txt

.PHONY:  all serv clean

all:	serv
		@for i in $(DIRS) ; do \
			$(MAKE) -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done;

serv:	$(SERVICES)
		make -C .. mounthdd
		sudo cp $(SERVICES) $(HDDMOUNT)/etc/services
		make -C .. umounthdd

clean:
		@for i in $(DIRS) ; do \
			$(MAKE) -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
