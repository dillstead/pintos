TEXIS = pintos.texi intro.texi tour.texi threads.texi userprog.texi	\
vm.texi filesys.texi references.texi standards.texi doc.texi		\
sample.tmpl devel.texi debug.texi 44bsd.texi

all: pintos.html pintos.info pintos.dvi pintos.ps pintos.pdf

pintos.html: $(TEXIS) texi2html
	./texi2html -toc_file=$@ -split=chapter -nosec_nav -nomenu -init_file pintos-t2h.init $<

pintos.info: $(TEXIS)
	makeinfo $<

pintos.dvi: $(TEXIS)
	texi2dvi $< -o $@

pintos.ps: pintos.dvi
	dvips $< -o $@

pintos.pdf: $(TEXIS)
	texi2pdf $< -o $@

clean:
	rm -f *.info *.html
	rm -f *.dvi *.pdf *.ps *.log *~
	rm -rf WWW

dist: pintos.html pintos.pdf
	rm -rf WWW
	mkdir WWW WWW/specs
	cp *.html *.pdf *.css *.tmpl WWW
	(cd ../specs && cp -r *.pdf freevga kbd sysv-abi-update.html ../doc/WWW/specs)
