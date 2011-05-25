DEP_START = $(wildcard $(ROOT)/build/$(ARCH)-dist/lib/gcc/$(TARGET)/4.4.3/crt*.o)
ifeq ($(ARCH),x86)
	DEP_DEFLIBS = $(ROOT)/build/$(ARCH)-dist/lib/libsupc++.a
else
	DEP_DEFLIBS =
endif
ifeq ($(LINKTYPE),static)
	DEP_DEFLIBS += $(wildcard $(ROOT)/build/$(ARCH)-dist/$(TARGET)/lib/lib*.a)
else
	DEP_DEFLIBS += $(filter-out $(ROOT)/build/$(ARCH)-dist/$(TARGET)/lib/libshlibtest.so \
		$(ROOT)/build/$(ARCH)-dist/$(TARGET)/lib/libshlibtest2.so, \
		$(wildcard $(ROOT)/build/$(ARCH)-dist/$(TARGET)/lib/lib*.so))
endif