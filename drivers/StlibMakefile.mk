ROOT = ../..
BUILDL = $(BUILD)/drivers/$(NAME)
BIN = $(BUILD)/driver_$(NAME).bin
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")
APP = $(NAME).app
APPCPY = $(BUILD)/apps/$(APP)

CFLAGS = -static -Wl,-Bstatic $(CDEFFLAGS) $(ADDFLAGS)

# sources
CSRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c")

# objects
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))

.PHONY: all clean

-include $(ROOT)/sysdeps.mk

all:	$(BUILDDIRS) $(APPCPY) $(BIN)

$(BIN):	$(DEP_START) $(DEP_DEFLIBS) $(COBJ) $(ADDLIBS)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -o $(BIN) $(COBJ) $(ADDLIBS);
		@echo "	" COPYING ON DISK
		$(ROOT)/tools/disk.sh copy $(BIN) /sbin/$(NAME)

$(APPCPY): $(APP)
		$(ROOT)/tools/disk.sh copy $(APP) /apps/$(NAME)
		cp $(APP) $(APPCPY)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
		rm -f $(APPCPY) $(BIN) $(COBJ) $(DEPS)
