SRCDIR = ../../src

SINGLETONS = \
	$(addprefix create-, normal empty null bad-ptr long exists bound) \
	$(addprefix args-, argc argv0 argvn single multiple dbl-space)

define SINGLETON_PROG
PROGS += $(1)
$(1)_SRC = $(1).c
endef

$(foreach prog,$(SINGLETONS),$(eval $(call SINGLETON_PROG,$(prog))))
DISKS = $(patsubst %,%.dsk,$(PROGS))

disks: $(DISKS)

PINTOS = ../../src/utils/pintos
OS_DISK = ../../src/userprog/build/os.dsk
%.dsk: %
	rm -f $@.tmp
	$(PINTOS) make-disk $@.tmp 2
	$(PINTOS) -v --os-disk=$(OS_DISK) --fs-disk=$@.tmp run -f -q
	$(PINTOS) -v --os-disk=$(OS_DISK) --fs-disk=$@.tmp put $<
	mv $@.tmp $@

clean::
	rm -f $(DISKS)

include $(SRCDIR)/Makefile.userprog

CFLAGS += -Werror
