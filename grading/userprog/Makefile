SRCDIR = ../../src

SINGLETONS = create-normal create-empty create-null create-invalid create-long 

define SINGLETON_PROG
PROGS += $(1)
$(1)_SRC = $(1).c
endef

$(foreach prog,$(SINGLETONS),$(eval $(call SINGLETON_PROG,$(prog))))
DISKS = $(patsubst %,%.dsk,$(PROGS))

disks: $(DISKS)

PINTOS = ../../src/utils/pintos
%.dsk: % os.dsk
	rm -f $@ fs.dsk
	$(PINTOS) make-disk fs.dsk 2
	$(PINTOS) -v run -f -q
	$(PINTOS) -v put $<
	mv fs.dsk $@

os.dsk: ../../src/userprog/build/os.dsk
	ln -sf $< $@

clean::
	rm -f $(DISKS)

include $(SRCDIR)/Makefile.userprog
