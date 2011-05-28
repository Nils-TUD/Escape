ROOT = ..
HDD = $(BUILD)/hd.img
DISKMNT = $(ROOT)/diskmnt
CD = $(BUILD)/cd.iso
SUDO = sudo
DIRS = arch/$(ARCH)

BUILD_KERNEL=$(wildcard $(BUILD)/kernel*.bin)
BUILD_BIN=$(wildcard $(BUILD)/user_*.bin)
BUILD_SBIN=$(wildcard $(BUILD)/driver_*.bin)
BUILD_LIB=$(wildcard $(BUILD)/lib*.so)
#BUILD_LIBGCC=$(BUILD)/../../../toolchain/$(ARCH)/$(TARGET)/lib/libgcc_s.so.1
BUILD_ETC=$(shell find $(ROOT)/dist -type f | grep -v '\.svn\|Makefile\|/boot/')

DISK_KERNEL=$(patsubst $(BUILD)/kernel%.bin,$(DISKMNT)/boot/kernel%.bin,$(BUILD_KERNEL))
DISK_BIN=$(patsubst $(BUILD)/user_%.bin,$(DISKMNT)/bin/%,$(BUILD_BIN))
DISK_SBIN=$(patsubst $(BUILD)/driver_%.bin,$(DISKMNT)/sbin/%,$(BUILD_SBIN))
DISK_LIB=$(patsubst $(BUILD)/lib%.so,$(DISKMNT)/lib/lib%.so,$(BUILD_LIB))
DISK_LIBGCC=$(DISKMNT)/lib/libgcc_s.so.1

.PHONY: all cd hdd

all:
		@for i in $(DIRS) ; do \
			$(MAKE) -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

cd:	$(CD)

hdd:	$(HDD)

$(CD): $(BUILD_KERNEL) $(BUILD_BIN) $(BUILD_SBIN) $(BUILD_LIB) $(BUILD_LIBGCC) $(BUILD_ETC)
		@echo "	" UPDATING $(CD)
		@$(ROOT)/tools/iso.sh

$(HDD): $(BUILD_KERNEL) $(BUILD_BIN) $(BUILD_SBIN) $(BUILD_LIB) $(BUILD_LIBGCC)
		@echo "	" UPDATING $(HDD)
		@$(ROOT)/tools/disk.sh mountp1
		@#$(SUDO) cp $(BUILD_KERNEL) $(DISKMNT)/boot
		@for i in $(BUILD_BIN); do \
			$(SUDO) cp $$i $(DISKMNT)/bin/`echo $$i | sed "s/.*\/user_\(.*\)\.bin/\1/g"`; \
		done;
		@for i in $(BUILD_SBIN); do \
			$(SUDO) cp $$i $(DISKMNT)/sbin/`echo $$i | sed "s/.*\/driver_\(.*\)\.bin/\1/g"`; \
		done;
		@$(SUDO) cp -f $(BUILD_LIB) $(DISKMNT)/lib
		@#$(SUDO) cp $(BUILD)/../../../toolchain/$(ARCH)/$(TARGET)/lib/libgcc_s.so.1 $(DISKMNT)/lib/libgcc_s.so.1
		@$(ROOT)/tools/disk.sh unmount
		@touch $(HDD)

clean:
		@for i in $(DIRS) ; do \
			$(MAKE) -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

#pre-hdd:
#		@echo "	" UPDATING $(HDD)
#		@$(ROOT)/tools/disk.sh mountp1

#post-hdd:
#		@$(ROOT)/tools/disk.sh unmount
#		@touch $(HDD)

#$(DISKMNT)/boot/kernel.bin:	$(BUILD)/kernel.bin
#		$(SUDO) cp $< $@

#$(DISKMNT)/boot/kernel_test.bin:	$(BUILD)/kernel_test.bin
#		$(SUDO) cp $< $@

#$(DISKMNT)/bin/%:	$(BUILD)/user_%.bin
#		$(SUDO) cp $< $@

#$(DISKMNT)/sbin/%:	$(BUILD)/driver_%.bin
#		$(SUDO) cp $< $@

#$(DISKMNT)/lib/lib%.so:	$(BUILD)/lib%.so
#		$(SUDO) cp $< $@

#$(DISKMNT)/lib/libgcc_s.so.1:	$(BUILD_LIBGCC)
#		$(SUDO) cp $< $@
