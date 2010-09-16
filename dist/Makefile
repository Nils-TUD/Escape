ROOT = ..
HDD = $(BUILD)/hd.img
DISKMNT = $(ROOT)/diskmnt
CD = $(BUILD)/cd.iso
SUDO = sudo

BUILD_KERNEL=$(wildcard $(BUILD)/kernel*.bin)
BUILD_BIN=$(wildcard $(BUILD)/user_*.bin)
BUILD_SBIN=$(wildcard $(BUILD)/driver_*.bin)
BUILD_LIB=$(wildcard $(BUILD)/lib*.so)
BUILD_LIBGCC=$(BUILD)/../dist/i586-elf-escape/lib/libgcc_s.so.1

DISK_KERNEL=$(patsubst $(BUILD)/kernel%.bin,$(DISKMNT)/boot/kernel%.bin,$(BUILD_KERNEL))
DISK_BIN=$(patsubst $(BUILD)/user_%.bin,$(DISKMNT)/bin/%,$(BUILD_BIN))
DISK_SBIN=$(patsubst $(BUILD)/driver_%.bin,$(DISKMNT)/sbin/%,$(BUILD_SBIN))
DISK_LIB=$(patsubst $(BUILD)/lib%.so,$(DISKMNT)/lib/lib%.so,$(BUILD_LIB))
DISK_LIBGCC=$(DISKMNT)/lib/libgcc_s.so.1

.PHONY: all clean

all:	$(HDD) $(CD)

$(CD): $(BUILD_KERNEL) $(BUILD_BIN) $(BUILD_SBIN) $(BUILD_LIB) $(BUILD_LIBGCC)
		@echo "	" UPDATING $(CD)
		@$(ROOT)/tools/iso.sh

$(HDD): pre-hdd $(DISK_KERNEL) $(DISK_BIN) $(DISK_SBIN) $(DISK_LIB) $(DISK_LIBGCC) post-hdd

pre-hdd:
		@echo "	" UPDATING $(HDD)
		@$(ROOT)/tools/disk.sh mountp1

post-hdd:
		@$(ROOT)/tools/disk.sh unmount
		@touch $(HDD)

$(DISKMNT)/boot/kernel.bin:	$(BUILD)/kernel.bin
		@$(SUDO) cp $< $@

$(DISKMNT)/boot/kernel_test.bin:	$(BUILD)/kernel_test.bin
		@$(SUDO) cp $< $@

$(DISKMNT)/bin/%:	$(BUILD)/user_%.bin
		@$(SUDO) cp $< $@

$(DISKMNT)/sbin/%:	$(BUILD)/driver_%.bin
		@$(SUDO) cp $< $@

$(DISKMNT)/lib/lib%.so:	$(BUILD)/lib%.so
		@$(SUDO) cp $< $@

$(DISKMNT)/lib/libgcc_s.so.1:	$(BUILD_LIBGCC)
		@$(SUDO) cp $< $@

clean:
