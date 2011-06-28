NAME=init
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libusergroup.a
else
	ADDFLAGS = -lusergroup
endif

-include ../DefMakefile.mk