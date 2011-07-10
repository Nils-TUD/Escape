NAME = shell
LIBDEPS += $(BUILD)/libshell.a
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libshell.a
else
	ADDFLAGS = -lshell
endif

-include ../SimpleMakefile.mk
