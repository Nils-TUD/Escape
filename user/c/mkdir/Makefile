NAME=mkdir
ifeq ($(LINKTYPE),static)
ADDFLAGS=-I../../../libesc/include
ADDLIBS=$(BUILD)/libesc.a
else
ADDFLAGS=-lesc -I../../../libesc/include
endif

-include ../DefMakefile.mk