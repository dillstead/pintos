SRCDIR = ../../src

SINGLETONS = create-normal create-empty create-null create-invalid create-long 

define SINGLETON_PROG
PROGS += $(1)
$(1)_SRC = $(1).c
endef

$(foreach prog,$(SINGLETONS),$(eval $(call SINGLETON_PROG,$(prog))))

include $(SRCDIR)/Makefile.userprog
