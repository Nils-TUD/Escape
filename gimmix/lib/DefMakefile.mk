BUILD = ../../build
BUILDL = $(BUILD)/lib/$(NAME)
LIB = $(BUILD)/lib$(NAME).a
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,$(BUILDL)/%.o,$(SRCS))

.PHONY:		all clean

all:	$(BUILDDIRS) $(LIB)

$(BUILDDIRS):
	@for i in $(BUILDDIRS); do \
		if [ ! -d $$i ]; then mkdir -p $$i; fi \
	done;

$(LIB):	$(OBJS)
	@echo "	" AR $@
	@$(AR) cr $@ $(OBJS)

$(BUILDL)/%.o:	%.c
	@echo "	" CC $<
	@$(CC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) $(LIB)
