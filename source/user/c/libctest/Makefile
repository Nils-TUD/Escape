NAME=libctest
SUBDIR=tests
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libusergroup.a
else
	ADDFLAGS = -lusergroup
endif

-include ../RecMakefile.mk
