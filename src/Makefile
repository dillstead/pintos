SUBDIRS = threads userprog vm filesys

all::
	@echo "Run 'make' in subdirectories $(SUBDIRS)."
	@echo "This top-level make has only 'clean' targets."

clean::
	for d in $(SUBDIRS) tests; do $(MAKE) -C $$d $@; done

distclean:: clean
	find . -name '*~' -exec rm '{}' \;

TAGS::
	etags --members *.[chS] `find $(SUBDIRS) -name \*.[chS]`

tags::
	ctags -T *.[chS] `find . -name \*.[chS]`

