SUBDIRS = src grading doc tests

all::
	@echo "This makefile has only 'clean' targets."

clean::
	for d in $(SUBDIRS); do $(MAKE) -C $$d $@; done

distclean:: clean
	find . -name '*~' -exec rm '{}' \;

check::
	make -C tests $@
