DIST=$(ROOT)/../../escape/toolchain
DEP_START = $(wildcard $(DIST)/$(ARCH)/lib/gcc/$(TARGET)/$(GCCVER)/crt*.o)
ifeq ($(ARCH),i586)
	DEP_DEFLIBS = $(DIST)/$(ARCH)/lib/libsupc++.a
else
	DEP_DEFLIBS =
endif
ifeq ($(LINKTYPE),static)
	DEP_DEFLIBS += $(wildcard $(DIST)/$(ARCH)/$(TARGET)/lib/lib*.a)
else
	DEP_DEFLIBS += $(filter-out $(DIST)/$(ARCH)/$(TARGET)/lib/libshlibtest.so \
		$(DIST)/$(ARCH)/$(TARGET)/lib/libshlibtest2.so, \
		$(wildcard $(DIST)/$(ARCH)/$(TARGET)/lib/lib*.so))
endif