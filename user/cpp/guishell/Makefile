NAME = guishell
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libshell.a $(BUILD)/libvterm.a
else
	ADDFLAGS = -lshell -lvterm
endif

-include ../DefMakefile.mk
