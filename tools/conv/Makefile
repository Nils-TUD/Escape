NAME = conv
BUILDL = $(BUILD)/tools/$(NAME)
BIN = $(BUILD)/$(NAME)
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")

CC = g++
CFLAGS = $(CPPDEFFLAGS)

# sources
CSRC = $(shell find $(SUBDIRS) -name "*.cpp")

# objects
COBJ = $(patsubst %.cpp,$(BUILDL)/%.o,$(CSRC))

.PHONY: all clean

all:	$(BIN)

$(BIN):	$(BUILDDIRS) $(COBJ)
		@echo "	" LD $(BIN);
		$(CC) $(CFLAGS) -o $(BIN) $(COBJ);

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o:		%.cpp
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
		rm -f $(BIN) $(COBJ) $(DEPS)
