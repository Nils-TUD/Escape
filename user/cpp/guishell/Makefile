NAME = guishell
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libvterm.a $(BUILD)/libshell.a
else
	ADDFLAGS = -lvterm -lshell
endif

-include ../DefMakefile.mk
