NAME = shell
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libshell.a
else
	ADDFLAGS = -lshell
endif

-include ../DefMakefile.mk
