SUBDIRS = userprog vm filesys

all::
	@echo "Run 'make' in subdirectories $(SUBDIRS)."
	@echo "This makefile has only 'clean' targets."

clean::
	for d in $(SUBDIRS); do $(MAKE) -C $$d $@; done

distclean:: clean
	find . -name '*~' -exec rm '{}' \;
