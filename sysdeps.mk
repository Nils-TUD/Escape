DEP_START = $(wildcard $(ROOT)/build/dist/lib/gcc/i586-elf-escape/4.4.3/crt*.o)
DEP_DEFLIBS = $(ROOT)/build/dist/lib/libsupc++.a
ifeq ($(LINKTYPE),static)
	DEP_DEFLIBS += $(wildcard $(ROOT)/build/dist/i586-elf-escape/lib/lib*.a)
else
	DEP_DEFLIBS += $(filter-out $(ROOT)/build/dist/i586-elf-escape/lib/libshlibtest.so, \
		$(wildcard $(ROOT)/build/dist/i586-elf-escape/lib/lib*.so))
endif