NAME = stdc++
ADDFLAGS = -nodefaultlibs
ADDLIBS = -lc
LIBDEPS += $(BUILD)/libc.a
ifeq ($(LIBGCC),static)
	ADDLIBS += -lgcc -lgcc_eh
endif

-include ../DefCppMakefile.mk
