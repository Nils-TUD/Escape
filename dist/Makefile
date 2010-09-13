ROOT = ..
HDD = $(BUILD)/hd.img
DISKMNT = $(ROOT)/diskmnt
CD = $(BUILD)/cd.iso
SUDO = sudo

DISK_KERNEL=$(wildcard $(BUILD)/kernel*.bin)
DISK_BIN=$(wildcard $(BUILD)/user_*.bin)
DISK_SBIN=$(wildcard $(BUILD)/driver_*.bin)
DISK_LIB=$(wildcard $(BUILD)/lib*.so)

.PHONY: all clean

all:	$(HDD) $(CD)

$(CD): $(DISK_KERNEL) $(DISK_BIN) $(DISK_SBIN) $(DISK_LIB)
		@echo "	" UPDATING $(CD)
		@$(ROOT)/tools/iso.sh

$(HDD): $(DISK_KERNEL) $(DISK_BIN) $(DISK_SBIN) $(DISK_LIB)
		@echo "	" UPDATING $(HDD)
		@$(ROOT)/tools/disk.sh mountp1
		@$(SUDO) cp $(DISK_KERNEL) $(DISKMNT)/boot
		@for i in $(DISK_BIN); do \
			$(SUDO) cp $$i $(DISKMNT)/bin/`echo $$i | sed "s/.*\/user_\(.*\)\.bin/\1/g"`; \
		done;
		@for i in $(DISK_SBIN); do \
			$(SUDO) cp $$i $(DISKMNT)/sbin/`echo $$i | sed "s/.*\/driver_\(.*\)\.bin/\1/g"`; \
		done;
		@$(SUDO) cp -f $(DISK_LIB) $(DISKMNT)/lib
		@$(SUDO) cp $(BUILD)/../dist/i586-elf-escape/lib/libgcc_s.so.1 $(DISKMNT)/lib/libgcc_s.so.1
		@$(ROOT)/tools/disk.sh unmount
		@touch $(HDD)

clean:
