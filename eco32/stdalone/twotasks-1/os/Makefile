BUILD = ../../../build

SRC = start.s main.c end.s
BIN = twotasks-1.bin
MAP = twotasks-1.map

all:		$(BIN)

run:		$(BIN)
		$(BUILD)/bin/sim -i -t 1 -l $(BIN)

$(BIN):		$(SRC) task1.dump task2.dump
		$(BUILD)/bin/lcc -A -Wo-kernel \
		  -Wl-m -Wl$(MAP) -o $(BIN) $(SRC)

task1.dump:
		../dump/dump ../task1/task1 task1.dump

task2.dump:
		../dump/dump ../task2/task2 task2.dump

clean:
		rm -f *~ task1.dump task2.dump $(BIN) $(MAP)
