SUBDIRS = threads userprog vm filesys

all::
	@echo "Run 'make' in subdirectories $(SUBDIRS)."
	@echo "This top-level make has only 'clean' targets."

clean::
	for d in $(SUBDIRS); do $(MAKE) -C $$d $@; done

distclean::
	$(MAKE) clean
	find . -name '*~' -exec rm '{}' \;

TAGS::
	etags --members `find . -name \*.[chS]`

tags::
	ctags -T `find . -name \*.[chS]`

