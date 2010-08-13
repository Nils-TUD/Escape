NAME=gtest
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libgui.a
else
	ADDFLAGS = -lgui
endif

-include ../DefMakefile.mk