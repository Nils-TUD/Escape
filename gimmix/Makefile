BUILD = build
ORG = org
DIRS = lib tools testgen tests src unittests

export CPP = cpp
export LD = ld
export CC = gcc
export AR = ar
export CFLAGS = -rdynamic -O3 -DNDEBUG -g -Wall -std=c99 -pedantic \
 -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes \
 -Wmissing-declarations -Wnested-externs -Winline -Wstrict-prototypes

.PHONY: all runscript run debug runx debugx runm debugm runp debugp runpx debugpx test clean

all:
	@for i in $(DIRS); do \
		$(MAKE) -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
	done

start:	all
	$(BUILD)/gimmix $(PARAMS) -i

runscript:	all
	$(BUILD)/gimmix $(PARAMS) --script=tests/cli/$(SCRIPT).script

run:	all
	$(BUILD)/gimmix $(PARAMS) --user -l $(BUILD)/tests/$(PROG).mmx \
		--postcmds=`sed -n 1,1p tests/$(PROG).test`

debug:	all
	$(BUILD)/gimmix $(PARAMS) --user -i -l $(BUILD)/tests/$(PROG).mmx

runx:	all
	$(BUILD)/gimmix $(PARAMS) -l $(BUILD)/tests/$(PROG).mmx \
		--postcmds=`sed -n 1,1p tests/$(PROG).test`

debugx:	all
	$(BUILD)/gimmix $(PARAMS) -i -l $(BUILD)/tests/$(PROG).mmx

runm:	all
	make -C $(ORG)
	./$(ORG)/build/mmix $(BUILD)/tests/$(PROG).mmo --postcmds=`sed -n 1,1p tests/$(PROG).test`

debugm:	all
	make -C $(ORG)
	./$(ORG)/build/mmix -i -v $(BUILD)/tests/$(PROG).mmo

runp:	all
	make -C $(ORG)
	./$(ORG)/build/mmmix $(ORG)/plain.mmconfig $(BUILD)/tests/$(PROG).mmb \
		--postcmds=`sed -n 1,1p tests/$(PROG).test`

debugp:	all
	make -C $(ORG)
	./$(ORG)/build/mmmix $(ORG)/plain.mmconfig $(BUILD)/tests/$(PROG).mmb

runpx:	all
	make -C $(ORG)
	./$(ORG)/build/mmmix $(ORG)/plain.mmconfig $(BUILD)/tests/$(PROG).mmx \
		--postcmds=`sed -n 1,1p tests/$(PROG).test`

debugpx:	all
	make -C $(ORG)
	./$(ORG)/build/mmmix $(ORG)/plain.mmconfig $(BUILD)/tests/$(PROG).mmx

test:	all
	$(BUILD)/gimmixtest

clean:
	@for i in $(DIRS); do \
		$(MAKE) -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
	done
