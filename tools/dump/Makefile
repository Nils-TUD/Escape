NAME=dump
BUILD=../../build
BIN=$(BUILD)/$(NAME)

CC = g++
CFLAGS = $(CPPDEFFLAGS) 
CSRC=$(wildcard *.cpp)

COBJ=$(patsubst %.cpp,$(BUILD)/tools_$(NAME)_%.o,$(CSRC))

.PHONY: all clean

all:	$(BIN)

$(BIN):	$(COBJ)
		@echo "===== COMPILING & LINKING =====";
		$(CC) $(CFLAGS) -o $(BIN) $(COBJ);

$(BUILD)/tools_$(NAME)_%.o:		%.cpp
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

clean:
		rm -f $(BIN) $(COBJ)
