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
OBJS = $(patsubst src/%.cpp,$(BUILD)/libcpp_std_%.o,$(CSTDSRCS))
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

$(BUILD)/libcpp_std_%.o: src/%.cpp
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(STARTO): $(START)
		@echo "===== ASSEMBLING $(START) =====";
		nasm $(ASMFLAGS) -o $(STARTO) $(START)

$(DEP):	$(CSTDSRCS)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CSTDSRCS) > $(DEP)
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD)\/libcpp_)\1.o:/g" $(DEP);

-include $(DEP)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(OBJS) $(LIB) $(DEP) $(STARTO)
