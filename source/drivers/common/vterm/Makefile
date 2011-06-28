NAME=vterm
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libvterm.a $(BUILD)/libusergroup.a
else
	ADDFLAGS = -lvterm -lusergroup
endif

-include ../../ShlibSimpleMakefile.mk