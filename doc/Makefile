TEXIS = projects.texi intro.texi threads.texi mlfqs.texi userprog.texi	\
filesys.texi vm.texi standards.texi doc.texi devel.texi debug.texi

all: projects.html projects.info

projects.html: $(TEXIS) mlfqs1.png mlfqs2.png
	texi2html -toc_file=$@ -split=chapter -no-sec_nav -no-menu -init_file pintos-t2h.init $<

projects.info: $(TEXIS)
	makeinfo $<

%.eps: %.jgr
	jgraph $< > $@.tmp && mv $@.tmp $@

%.png: %.eps
	convert $< $@

clean:
	rm -f *.info *.html *.eps *.png
