BUILD=build
IMG=img
SRC=$(wildcard *.tex)
LIT=bibliography.bib
BST=unsrtnat-lastchecked.bst
MAIN=main.tex
PDF=$(BUILD)/$(subst .tex,.pdf,$(MAIN))
AUX=$(BUILD)/$(subst .tex,.aux,$(MAIN))
IMAGES=$(wildcard $(IMG)/*.*)
IMGPDFS=$(filter-out $(wildcard $(IMG)/*-crop.pdf),$(wildcard $(IMG)/*.pdf))
IMGCROPS=$(patsubst %.pdf,%-crop.pdf,$(IMGPDFS))
VIEWER=evince

.PHONY: all show clean

all:	$(PDF)

$(PDF):	$(SRC) $(LIT) $(BST) $(IMAGES) $(IMGCROPS)
	./texify.sh -output-directory=$(BUILD) -interaction=nonstopmode $(MAIN)
	xindy -L english -C utf8 -I xindy -M $(BUILD)/main.xdy -t $(BUILD)/main.glg \
		-o $(BUILD)/main.gls $(BUILD)/main.glo
	./texify.sh -output-directory=$(BUILD) -interaction=nonstopmode $(MAIN)
	xindy -L english -C utf8 -I xindy -M $(BUILD)/main.xdy -t $(BUILD)/main.glg \
		-o $(BUILD)/main.gls $(BUILD)/main.glo
	bibtex $(AUX)
	./texify.sh -output-directory=$(BUILD) -interaction=nonstopmode $(MAIN)
	./texify.sh -output-directory=$(BUILD) -interaction=nonstopmode $(MAIN)

$(IMG)/%-crop.pdf:	$(IMG)/%.pdf
	pdfcrop	$< $@

show:	$(PDF)
	$(VIEWER) $(PDF) &

clean:
	rm -f $(BUILD)/*
