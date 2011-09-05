BUILD = ../build
BUILDL = $(BUILD)/tests
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
MMIX = $(BUILD)/../org/build/mmix

# $(wildcard *.mms)
MMSFILES = $(shell find . -name "*.mms" | grep -v '\.svn')
MMPFILES = $(patsubst %.mms,$(BUILDL)/%.mmp,$(MMSFILES))
MMOFILES = $(patsubst %.mms,$(BUILDL)/%.mmo,$(MMSFILES))
MMXFILES = $(patsubst %.mms,$(BUILDL)/%.mmx,$(MMSFILES))
MMBFILES = $(patsubst %.mms,$(BUILDL)/%.mmb,$(MMSFILES))
BMMSFILES = $(patsubst %.mms,$(BUILDL)/%.mms,$(MMSFILES))
OTHER = $(BUILDL)/manual/hexmon.rom $(BUILDL)/manual/hellodisk.bin

.PHONY: all clean

all:	$(BUILDDIRS) $(MMOFILES) $(MMXFILES) $(MMBFILES) $(BMMSFILES) $(OTHER)

$(BUILDDIRS):
	@for i in $(BUILDDIRS); do \
		if [ ! -d $$i ]; then mkdir -p $$i; fi \
	done;

$(BUILDL)/manual/hexmon.rom:		$(BUILDL)/manual/hexmon.mmx
	$(BUILD)/gimmx2bin $< $@

$(BUILDL)/manual/hellodisk.bin:		$(BUILDL)/manual/helloworld.mmx
	$(BUILD)/gimmx2bin $< $@ --mbr

$(BUILDL)/%.mmx:	$(BUILDL)/%.mmo
	$(BUILD)/gimmo2mmx $< $@

$(BUILDL)/%.mmo:	$(BUILDL)/%.mmp
	$(BUILD)/gimmixal -b 100 -o $@ -l $(patsubst %.mmp,%.lst,$<) $<

$(BUILDL)/%.mmb:	$(BUILDL)/%.mmo
	$(MMIX) -D$@ $<

$(BUILDL)/%.mmp:	%.mms
	$(CPP) -C -H $< $@ -MD -MT $@

$(BUILDL)/%.mms:	%.mms
	cp $< $@

-include $(DEPS)

clean:
	rm -f $(MMXFILES) $(MMOFILES) $(MMPFILES) $(MMBFILES) $(BMMSFILES) $(OTHER)
