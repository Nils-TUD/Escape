NAME=vesatext
LIBDEPS += $(BUILD)/libvbe.a
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libvbe.a
else
	ADDFLAGS = -lvbe
endif

-include ../../ShlibSimpleMakefile.mk