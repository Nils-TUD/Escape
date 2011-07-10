NAME=groups
LIBDEPS += $(BUILD)/libusergroup.a
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libusergroup.a
else
	ADDFLAGS = -lusergroup
endif

-include ../SimpleMakefile.mk