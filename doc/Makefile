TEXIS = projects.texi threads.texi userprog.texi filesys.texi vm.texi

projects.info: $(TEXIS)
	makeinfo $<

projects.html: $(TEXIS)
	texi2html -toc_file=$@ -split=chapter -no-sec_nav -no-menu $<
