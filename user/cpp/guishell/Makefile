NAME = guishell
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libshell.a $(BUILD)/libvterm.a
	ADDFLAGS = -I../../../lib
else
	ADDFLAGS = -I../../../lib -lshell -lvterm
endif

-include ../DefMakefile.mk
