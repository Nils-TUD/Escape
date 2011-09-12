BUILD = ../../build
BUILDL = $(BUILD)/tools/$(NAME)
BIN = $(BUILD)/$(NAME)
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,$(BUILDL)/%.o,$(SRCS))

.PHONY:		all clean

all:	$(BUILDDIRS) $(BIN)

$(BUILDDIRS):
	@for i in $(BUILDDIRS); do \
		if [ ! -d $$i ]; then mkdir -p $$i; fi \
	done;

$(BIN):	$(OBJS) $(LIBS)
	@echo "	" LINKING $@
	@$(CC) $(CFLAGS) -o $@ -lm $(OBJS) $(LIBS)

$(BUILDL)/%.o:	%.c
	@echo "	" CC $<
	@$(CC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) $(BIN)
