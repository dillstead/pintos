TEXIS = pintos.texi intro.texi tour.texi threads.texi mlfqs.texi	\
userprog.texi vm.texi filesys.texi references.texi standards.texi	\
doc.texi devel.texi debug.texi

all: pintos.html pintos.info pintos.dvi pintos.ps pintos.pdf

pintos.html: $(TEXIS) mlfqs1.png mlfqs2.png
	./texi2html -toc_file=$@ -split=chapter -nosec_nav -nomenu -init_file pintos-t2h.init -nonumber $<

pintos.info: $(TEXIS)
	makeinfo $<

pintos.dvi: $(TEXIS) mlfqs1.eps mlfqs2.eps
	texi2dvi $< -o $@

pintos.ps: pintos.dvi
	dvips $< -o $@

pintos.pdf: $(TEXIS) mlfqs1.pdf mlfqs2.pdf
	texi2pdf $< -o $@

%.eps: %.jgr
	(jgraph $< > $@.tmp && mv $@.tmp $@) || touch $@

%.png: %.eps
	convert $< $@

%.pdf: %.eps
	epstopdf $< --outfile=$@

clean:
	rm -f *.info *.html *.png
	rm -f *.dvi *.pdf *.ps *.log
	rm -rf WWW

dist: pintos.html pintos.pdf
	rm -rf WWW
	mkdir WWW WWW/specs
	cp *.html *.png *.pdf *.css WWW
	(cd ../specs && cp -r *.pdf freevga kbd sysv-abi-update.html ../doc/WWW/specs)
