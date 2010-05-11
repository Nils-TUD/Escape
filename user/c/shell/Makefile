NAME = shell
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libshell.a
	ADDFLAGS = -I../../../lib
else
	ADDFLAGS = -I../../../lib -lshell
endif

-include ../DefMakefile.mk
