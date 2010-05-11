NAME=vesatext
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libvbe.a
	ADDFLAGS = -I../../lib
else
	ADDFLAGS = -I../../lib -lvbe
endif

-include ../ShlibMakefile.mk