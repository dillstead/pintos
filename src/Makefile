SUBDIRS = threads userprog vm filesys

all::
	@echo "Run 'make' in subdirectories $(SUBDIRS)."
	@echo "This top-level make has only 'clean' targets."

clean::
	for d in $(SUBDIRS) tests; do $(MAKE) -C $$d $@; done

distclean:: clean
	find . -name '*~' -exec rm '{}' \;

TAGS_SOURCES = `find \( -name tests -o -name build \) -prune -o -name \*.[chS] -print`

TAGS::
	etags --members $(TAGS_SOURCES)

tags::
	ctags -T --no-warn $(TAGS_SOURCES)

