SRCDIR = ../../src

# Main test programs.
TESTS = \
	$(addprefix args-, argc argv0 argvn single multiple dbl-space) \
	$(addprefix sc-, bad-sp bad-arg boundary) \
	halt exit \
	$(addprefix create-, normal empty null bad-ptr long exists bound) \
	$(addprefix open-, normal missing boundary empty null bad-ptr twice) \
	$(addprefix close-, normal twice stdin stdout bad-fd) \
	$(addprefix read-, normal bad-ptr boundary zero stdout bad-fd) \
	$(addprefix write-, normal bad-ptr boundary zero stdin bad-fd) \
	$(addprefix exec-, once arg multiple missing bad-ptr) \
	$(addprefix join-, simple twice killed bad-pid)

define TEST_PROG
PROGS += $(1)
$(subst -,_,$(1))_SRC = $(1).c
endef

$(foreach prog,$(TESTS),$(eval $(call TEST_PROG,$(prog))))
DISKS = $(patsubst %,%.dsk,$(PROGS))

disks: $(DISKS)

# Other programs needed by some of the main test programs. 
PROGS += child-simple child-arg child-bad
child_simple_SRC = child-simple.c
child_arg_SRC = child-arg.c
child_bad_SRC = child-bad.c
exec-once.dsk exec-multiple.dsk join-simple.dsk join-twice.dsk: child-simple
exec-arg.dsk: child-arg
join-killed.dsk: child-bad

%.dsk: %
	./prep-disk $<

clean::
	rm -f $(DISKS)

include $(SRCDIR)/Makefile.userprog

CFLAGS += -Werror
