BUILD = ../build
INC = ../include
SRC = ../src
BUILDL = $(BUILD)/unittests
BINNAME = gimmixtest
BIN = $(BUILD)/$(BINNAME)
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
SRCSUBDIRS = $(shell find $(SRC) -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS)) $(addprefix $(BUILDL)/foo/,$(SRCSUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
LIBS = $(BUILD)/libgetline.a $(BUILD)/libmmix.a -lX11 -lpthread -lm

CSRC = $(filter-out $(SRC)/main.c,$(shell find $(SRC) -name "*.c"))
TSRC = $(shell find . -name "*.c")
COBJ = $(patsubst %.c,$(BUILDL)/foo/%.o,$(CSRC))
TOBJ = $(patsubst %.c,$(BUILDL)/%.o,$(TSRC))

CFLAGS += -L/usr/X11R6/lib

.PHONY: all clean

all:	$(BUILDDIRS) $(BIN)

$(BUILDDIRS):
	@for i in $(BUILDDIRS); do \
		if [ ! -d $$i ]; then mkdir -p $$i; fi \
	done;

$(BIN):	$(COBJ) $(TOBJ) $(LIBS)
	@echo "	" LINKING $@
	@$(CC) $(CFLAGS) -o $@ $(COBJ) $(TOBJ) $(LIBS)

$(BUILDL)/%.o:	%.c
	@echo "	" CC $<
	@$(CC) $(CFLAGS) -I $(INC) -o $@ -c $< -MD

$(BUILDL)/foo/%.o:	%.c
	@echo "	" CC $<
	@$(CC) $(CFLAGS) -I $(INC) -o $@ -c $< -MD

-include $(DEPS)

clean:
	rm -f $(BIN) $(COBJ) $(TOBJ) $(DEPS)
