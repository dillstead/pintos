include ../../Make.config

SHELL = /bin/sh

VPATH = ../..

DEFINES = -DUSER
CPPFLAGS = -nostdinc -I../.. -I- -I../../lib -I../../lib/user \
	   -include constants.h

# Test programs to compile, and a list of sources for each.
# To add a new test, put its name on the PROGS list
# and then add a name_SRC line that lists its source files.
PROGS = bubsort echo halt insult lineup matmult recursor shell
bubsort_SRC = bubsort.c
echo_SRC = echo.c
halt_SRC = halt.c
insult_SRC = insult.c
lineup_SRC = lineup.c
matmult_SRC = matmult.c
recursor_SRC = recursor.c
shell_SRC = shell.c

# Linker flags.
LDFLAGS = -nostdlib -static
LDLIBS = $(shell $(CC) -print-libgcc-file-name)

# Uncomment the line below to round up segment sizes to full pages for
# testing purposes only.
#LDFLAGS += -Wl,-T,fullpage.x

# C library sources linked into every test program.
LIB_SRC  = lib/debug.c			# Debug code.
LIB_SRC += lib/random.c			# Pseudo-random numbers.
LIB_SRC += lib/stdio.c			# I/O library.
LIB_SRC += lib/stdlib.c			# atoi()
LIB_SRC += lib/string.c			# String functions.
LIB_SRC += lib/user/syscall.c		# System calls.
LIB_SRC += lib/user/syscall-stub.S	# System call stub.
LIB_SRC += lib/user/console.c		# Console code.

LIB_OBJ = $(patsubst %.c,%.o,$(patsubst %.S,%.o,$(LIB_SRC)))
LIB_DEP = $(patsubst %.o,%.d,$(LIB_OBJ))
LIB = libc.a lib/user/entry.o

PROGS_SRC = $(foreach prog,$(PROGS),$($(prog)_SRC))
PROGS_OBJ = $(patsubst %.c,%.o,$(patsubst %.S,%.o,$(PROGS_SRC)))
PROGS_DEP = $(patsubst %.o,%.d,$(PROGS_OBJ))

all: $(PROGS)

define TEMPLATE
$(1)_OBJ = $(patsubst %.c,%.o,$(patsubst %.S,%.o,$($(1)_SRC)))
$(1): $$($(1)_OBJ) $$(LIB) | ./lib/user
endef

./lib/user:
	$(MKDIR) -p lib/user

$(foreach prog,$(PROGS),$(eval $(call TEMPLATE,$(prog))))

$(PROGS): $(LIB)

libc.a: $(LIB_OBJ)
	rm -f $@
	ar r $@ $^
	ranlib $@

clean:
	rm -f $(PROGS) $(PROGS_OBJ) $(PROGS_DEP)
	rm -f $(LIB_DEP) $(LIB_OBJ) lib/user/entry.[do] libc.a 

.PHONY: all clean

-include $(LIB_DEP) $(PROGS_DEP)
