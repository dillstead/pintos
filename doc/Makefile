TEXIS = projects.texi intro.texi tour.texi threads.texi mlfqs.texi	\
userprog.texi vm.texi filesys.texi references.texi standards.texi	\
doc.texi devel.texi debug.texi

all: projects.html projects.info

projects.html: $(TEXIS) mlfqs1.png mlfqs2.png
	./texi2html -toc_file=$@ -split=chapter -nosec_nav -nomenu -init_file pintos-t2h.init -nonumber $<

projects.info: $(TEXIS)
	makeinfo $<

%.eps: %.jgr
	(jgraph $< > $@.tmp && mv $@.tmp $@) || touch $@

%.png: %.eps
	convert $< $@

clean:
	rm -f *.info *.html *.png
