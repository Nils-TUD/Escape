NAME=vterm
LIBDEPS += $(BUILD)/libvterm.a $(BUILD)/libusergroup.a
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libvterm.a $(BUILD)/libusergroup.a
else
	ADDFLAGS = -lvterm -lusergroup
endif

-include ../../ShlibSimpleMakefile.mk