BUILD = ../build
DIRS = $(filter-out $(wildcard *.*) Makefile,$(wildcard *))
SERVICES = $(BUILD)/services.txt

.PHONY:  all clean

all:
		@for i in $(DIRS) ; do \
			$(MAKE) -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done;
		
		@echo "" > $(SERVICES)
		@for i in $(DIRS) ; do \
			echo "static u8 service_"$$i"[] = {" >> $(SERVICES); \
			echo "		#include \"../../build/service_$$i.dump\"" >> $(SERVICES); \
			echo "};" >> $(SERVICES); \
		done;
		@echo "" >> $(SERVICES)
		@echo "static u8 *services[] = {" >> $(SERVICES)
		@for i in $(DIRS) ; do \
			echo "	service_"$$i"," >> $(SERVICES); \
		done;
		@echo "};" >> $(SERVICES)

clean:
		@for i in $(DIRS) ; do \
			$(MAKE) -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
		rm -f *~
