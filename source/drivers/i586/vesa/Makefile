NAME=vesa
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libvbe.a
else
	ADDFLAGS = -lvbe
endif

-include ../../ShlibSimpleMakefile.mk