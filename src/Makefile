all::
	@echo "Run 'make' in the threads, userprog, filesys, or vm directory."
	@echo "This top-level make has only 'clean' targets."

clean::
	for d in threads; do $(MAKE) -C $$d $@; done

distclean::
	$(MAKE) clean
	find . -name '*~' -exec rm '{}' \;
