#
#   Makefile for MMIXware
#

#   Be sure that CWEB version 3.0 or greater is installed before proceeding!
#   In fact, CWEB 3.61 is recommended for making hardcopy or PDF documentation.

#   If you prefer optimization to debugging, change -g to something like -O:
CFLAGS = -g -O3

BUILD = build
TEX = tex
DOC = docs
APPS = apps
BUILDDIRS = $(BUILD) $(DOC) $(TEX)

WEBFILES = $(filter-out abstime.w,$(wildcard *.w))
CHANGEFILES =
TESTFILES = *.mms silly.run silly.out *.mmconfig *.mmix
MISCFILES = Makefile makefile.dos README mmix.mp mmix.1
ALL = $(WEBFILES) $(TESTFILES) $(MISCFILES)

CSRC = $(patsubst %.w,$(BUILD)/%.c,$(WEBFILES))
COBJ = $(patsubst %.w,$(BUILD)/%.o,$(WEBFILES))
TEXS = $(patsubst %.w,$(TEX)/%.tex,$(WEBFILES))
PDFS = $(patsubst %.w,$(DOC)/%.pdf,$(filter-out boilerplate.w,$(WEBFILES)))

#   Uncomment the second line if you use pdftex to bypass .dvi files:
#PDFTEX = dvipdfm
PDFTEX = pdftex

.PHONY: all run debug doc clean

all:    $(BUILDDIRS) $(BUILD)/mmixal $(BUILD)/mmix $(BUILD)/mmotype $(BUILD)/mmmix $(CSRC)

$(BUILDDIRS):
	@for i in $(BUILDDIRS); do \
		if [ ! -d $$i ]; then mkdir -p $$i; fi \
	done;

run:	$(BUILD)/mmixal $(BUILD)/mmix
	$(BUILD)/mmixal -x -b 250 -l $(BUILD)/$(APP).mml -o $(BUILD)/$(APP).mmo $(APPS)/$(APP).mms
	$(BUILD)/mmix $(BUILD)/$(APP).mmo || true

debug:	$(BUILD)/mmixal $(BUILD)/mmix
	$(BUILD)/mmixal -x -b 250 -l $(BUILD)/$(APP).mml -o $(BUILD)/$(APP).mmo $(APPS)/$(APP).mms
	$(BUILD)/mmix -i $(BUILD)/$(APP).mmo

doc:    $(PDFS)
	tex -output-directory $(TEX) $(TEX)/mmixal.tex
	tex -output-directory $(TEX) $(TEX)/mmix-sim.tex
	dvips -n13 $(TEX)/mmixal.dvi -o $(DOC)/mmixal-intro.ps
	dvips -n8 $(TEX)/mmix-sim.dvi -o $(DOC)/mmix-sim-intro.ps

$(BUILD)/%.c: %.w
	if test -r $*.ch; then ctangle $< $*.ch; else ctangle $<; fi
	mv $(patsubst %.w,%.c,$<) $(BUILD)
	if [ -f $(patsubst %.w,%.h,$<) ]; then mv $(patsubst %.w,%.h,$<) $(BUILD); fi

$(DOC)/%.pdf: $(TEX)/%.tex
	pdftex -output-directory $(TEX) $<
	mv $(TEX)/$*.pdf $@
	#dvips $(patsubst %.tex,%.dvi,$<) -o $@

$(TEX)/%.tex: %.w
	if test -r $*.ch; then cweave $< $*.ch $@; else cweave $< - $@; fi

%.o: %.c $(BUILD)/abstime
	$(BUILD)/abstime > $(BUILD)/abstime.h
	$(CC) $(CFLAGS) -c -o $@ $(patsubst %.o,%.c,$@)
	rm $(BUILD)/abstime.h

$(BUILD)/mmix-config.o: $(BUILD)/mmix-config.c $(BUILD)/mmix-pipe.c
	$(CC) $(CFLAGS) -c $(BUILD)/mmix-config.c -o $@

$(BUILD)/abstime: $(BUILD)/abstime.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/mmmix:	$(COBJ) postcmds.c
	$(CC) $(CFLAGS) -DMMMIX=1 $(BUILD)/mmmix.o $(BUILD)/mmix-arith.o $(BUILD)/mmix-pipe.o \
		$(BUILD)/mmix-config.o $(BUILD)/mmix-mem.o $(BUILD)/mmix-io.o postcmds.c -o $@

$(BUILD)/mmixal:	$(COBJ)
	$(CC) $(CFLAGS) $(BUILD)/mmixal.o $(BUILD)/mmix-arith.o -o $@

$(BUILD)/mmix:	$(COBJ) postcmds.c
	$(CC) $(CFLAGS) $(BUILD)/mmix-sim.o $(BUILD)/mmix-arith.o $(BUILD)/mmix-io.o postcmds.c -o $@

tarfile: $(ALL)
	tar cvf /tmp/mmix.tar $(ALL)
	gzip -9 /tmp/mmix.tar

clean:
	rm -f *~ *.h core $(TEX)/* $(DOC)/* $(BUILD)/*

