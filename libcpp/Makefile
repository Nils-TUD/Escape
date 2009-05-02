BUILD = ../build
LIBDIR = ../lib
LIBCDIR = ../libc

CC = g++
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CPPDEFFLAGS) -Iinclude -I$(LIBDIR)/h \
	-I$(LIBCDIR)/include

# for long long arithmetic
GCCLIBDIR = ../libgcc
GCCLIB = $(GCCLIBDIR)/_divdi3.object \
	$(GCCLIBDIR)/_moddi3.object \
	$(GCCLIBDIR)/_udivdi3.object \
	$(GCCLIBDIR)/_umoddi3.object
CSTDSRCS = $(wildcard src/*.cpp)
CESCSRCS = $(wildcard src/esc/*.cpp)
CGUISRCS = $(wildcard src/esc/gui/*.cpp)
OBJS = $(patsubst src/%.cpp,$(BUILD)/libcpp_std_%.o,$(CSTDSRCS))
OBJS += $(patsubst src/esc/%.cpp,$(BUILD)/libcpp_esc_%.o,$(CESCSRCS))
OBJS += $(patsubst src/esc/gui/%.cpp,$(BUILD)/libcpp_escgui_%.o,$(CGUISRCS))
LIBCOBJS = $(wildcard $(BUILD)/libc_*_*.o) $(GCCLIB)
LIB = $(BUILD)/libcpp.a
DEP = $(BUILD)/libcpp.dep
START = startup.s
STARTO = $(BUILD)/libcpp_startup.o

.PHONY:		all clean

all: $(LIB)

$(LIB): $(OBJS) $(STARTO) $(LIBCOBJS)
		@echo "===== BUILDING ARCHIVE =====";
		ar rcs $(LIB) $(LIBCOBJS) $(OBJS)

$(BUILD)/libcpp_escgui_%.o: src/esc/gui/%.cpp
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/libcpp_esc_%.o: src/esc/%.cpp
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/libcpp_std_%.o: src/%.cpp
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(STARTO): $(START)
		@echo "===== ASSEMBLING $(START) =====";
		nasm $(ASMFLAGS) -o $(STARTO) $(START)

$(DEP):	$(CSTDSRCS) $(CESCSRCS) $(CGUISRCS)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CSTDSRCS) $(CESCSRCS) $(CGUISRCS) > $(DEP)
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		@for i in $(patsubst src/esc/gui/%.cpp,%,$(wildcard src/esc/gui/*.cpp)) ; do \
			sed --in-place -e "s/$$i.o:/$(subst /,\/,$(BUILD)/libcpp_escgui_)$$i.o:/g" $(DEP); \
		done;
		@for i in $(patsubst src/esc/%.cpp,%,$(wildcard src/esc/*.cpp)) ; do \
			sed --in-place -e "s/$$i.o:/$(subst /,\/,$(BUILD)/libcpp_esc_)$$i.o:/g" $(DEP); \
		done;
		@for i in $(patsubst src/%.cpp,%,$(wildcard src/*.cpp)) ; do \
			sed --in-place -e "s/$$i.o:/$(subst /,\/,$(BUILD)/libcpp_std_)$$i.o:/g" $(DEP); \
		done;

-include $(DEP)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(OBJS) $(LIB) $(DEP) $(STARTO)
