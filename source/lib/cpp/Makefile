NAME = stdc++
ADDFLAGS = -nodefaultlibs
ADDLIBS = -lc
ADDSTLIB = $(ROOT)/../toolchain/$(ARCH)/lib/libsupc++.a
ifeq ($(LIBGCC),static)
	ADDLIBS += -lgcc -lgcc_eh
endif

-include ../DefCppMakefile.mk
