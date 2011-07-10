NAME = guishell
LIBDEPS += $(BUILD)/libgui.a $(BUILD)/libvterm.a $(BUILD)/libshell.a
ifeq ($(LINKTYPE),static)
	ADDLIBS += $(BUILD)/libgui.a $(BUILD)/libvterm.a $(BUILD)/libshell.a
else
	ADDFLAGS = -lgui -lvterm -lshell
endif

-include ../DefMakefile.mk
