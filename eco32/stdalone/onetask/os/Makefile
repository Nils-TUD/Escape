BUILD = ../../../build

SRC = start.s main.c end.s
BIN = onetask.bin
MAP = onetask.map

all:		$(BIN)

run:		$(BIN)
		$(BUILD)/bin/sim -i -t 1 -l $(BIN)

$(BIN):		$(SRC) task.dump
		$(BUILD)/bin/lcc -A -Wo-kernel \
		  -Wl-m -Wl$(MAP) -o $(BIN) $(SRC)

task.dump:
		../dump/dump ../task/task task.dump

clean:
		rm -f *~ task.dump $(BIN) $(MAP)
