# -*- Makefile -*-

FABS=pi-adapter-fab.zip termination-fab.zip

all : $(FABS)

%.zip : %.kicad_pcb
	python ./kicad-fab.py $*
	zip -r $@ $*

%-fab.kicad_pcb : %.kicad_pcb
	sed "s/%%gitversion%%/`git log --date=short --pretty=format:'%h@%cd' -n 1 $<`/" < $^ > $@

clean:
	rm -rf $(FABS) $(FABS:.zip=)
