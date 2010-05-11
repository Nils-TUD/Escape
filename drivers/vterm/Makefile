NAME=vterm
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libvterm.a
	ADDFLAGS = -I../../lib
else
	ADDFLAGS = -I../../lib -lvterm
endif

-include ../ShlibMakefile.mk