DIST=$(ROOT)/../../escape/toolchain
DEP_START = $(wildcard $(DIST)/$(ARCH)/lib/gcc/$(TARGET)/$(GCCVER)/crt*.o)
ifeq ($(ARCH),i586)
	DEP_DEFLIBS = $(DIST)/$(ARCH)/lib/libsupc++.a
else
	DEP_DEFLIBS =
endif