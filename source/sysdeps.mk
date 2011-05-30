DEP_START = $(wildcard $(ROOT)/../toolchain/$(ARCH)/lib/gcc/$(TARGET)/$(GCCVER)/crt*.o)
ifeq ($(ARCH),i586)
	DEP_DEFLIBS = $(ROOT)/../toolchain/$(ARCH)/lib/libsupc++.a
else
	DEP_DEFLIBS =
endif
ifeq ($(LINKTYPE),static)
	DEP_DEFLIBS += $(wildcard $(ROOT)/../toolchain/$(ARCH)/$(TARGET)/lib/lib*.a)
else
	DEP_DEFLIBS += $(filter-out $(ROOT)/../toolchain/$(ARCH)/$(TARGET)/lib/libshlibtest.so \
		$(ROOT)/../toolchain/$(ARCH)/$(TARGET)/lib/libshlibtest2.so, \
		$(wildcard $(ROOT)/../toolchain/$(ARCH)/$(TARGET)/lib/lib*.so))
endif