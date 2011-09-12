BUILD=build
SRC=$(wildcard *.tex)
MAIN=main.tex
PDF=$(BUILD)/$(subst .tex,.pdf,$(MAIN))
AUX=$(BUILD)/$(subst .tex,.aux,$(MAIN))
VIEWER=evince

.PHONY: all show clean

all:	$(PDF)

$(PDF):	$(SRC)
	./texify.sh -output-directory=$(BUILD) -interaction=nonstopmode $(MAIN)

show:	$(PDF)
	$(VIEWER) $(PDF) &

clean:
	rm -f $(BUILD)/*
