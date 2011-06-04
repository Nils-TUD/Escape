NAME=vterm
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libvterm.a
else
	ADDFLAGS = -lvterm
endif

-include ../../ShlibSimpleMakefile.mk