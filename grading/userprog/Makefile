SRCDIR = ../../src

SINGLETONS = \
	$(addprefix args-, argc argv0 argvn single multiple dbl-space) \
	$(addprefix sc-, bad-sp bad-arg boundary) \
	halt exit \
	$(addprefix create-, normal empty null bad-ptr long exists bound) \
	$(addprefix open-, normal missing boundary empty null bad-ptr twice) \
	$(addprefix close-, normal twice stdin stdout bad-fd) \
	$(addprefix read-, normal bad-ptr boundary zero stdout bad-fd)

define SINGLETON_PROG
PROGS += $(1)
$(subst -,_,$(1))_SRC = $(1).c
endef

$(foreach prog,$(SINGLETONS),$(eval $(call SINGLETON_PROG,$(prog))))
DISKS = $(patsubst %,%.dsk,$(PROGS))

disks: $(DISKS)

%.dsk: %
	./prep-disk $<

clean::
	rm -f $(DISKS)

include $(SRCDIR)/Makefile.userprog

CFLAGS += -Werror
