SHELL = /bin/sh
VPATH = $(TOP_SRCDIR)/threads:$(TOP_SRCDIR)/devices:$(TOP_SRCDIR)/lib

-include *.d

DEFINES =
WARNINGS = -Wall -W -Wstrict-prototypes -Wmissing-prototypes
CFLAGS = -g -O3 -MMD $(WARNINGS) $(INCLUDES) $(DEFINES)
ASFLAGS = $(INCLUDES) $(DEFINES)

# Core kernel.
THREADS_SRC  = start.S	# Must be first.
THREADS_SRC += init.c	# Start-up code.
THREADS_SRC += thread.c	# Thread management core.
THREADS_SRC += switch.S	# Thread switch routine.
THREADS_SRC += interrupt.c	# Interrupt core.
THREADS_SRC += intr-stubs.S	# Interrupt stubs.
THREADS_SRC += synch.c	# Synchronization.
THREADS_SRC += palloc.c	# Page allocator.
THREADS_SRC += malloc.c	# Subpage allocator.

# Device driver code.
DEVICES_SRC  = timer.c	# Timer device.
DEVICES_SRC += kbd.c	# Keyboard device.
DEVICES_SRC += vga.c	# Video device.
DEVICES_SRC += serial.c	# Serial port device.

# Library code.
LIB_SRC  = debug.c		# Debug helpers.
LIB_SRC += lib.c		# Standard C library.
LIB_SRC += random.c		# Pseudo-random numbers.
LIB_SRC += list.c		# Doubly-linked lists.
LIB_SRC += bitmap.c		# Bitmaps.

# Objects.
OBJECTS = $(patsubst %.c,%.o,$(patsubst %.S,%.o,$(SOURCES)))

all: diskimage.bin

intr-stubs.S: $(TOP_SRCDIR)/threads/intr-stubs.pl
	$< > $@.tmp && mv $@.tmp $@

kernel.o: $(OBJECTS)
	echo $(OBJECTS)
	ld -T $(TOP_SRCDIR)/threads/kernel.lds -o $@ $^ \
		`$(CC) -print-libgcc-file-name`

kernel.bin: kernel.o
	objcopy -O binary -R .note -R .comment -S $< $@

loader.bin: loader.S kernel.bin
	gcc -c $< -DKERNEL_LOAD_PAGES=`perl -e 'print int (((-s "kernel.bin") + 4095) / 4096);'`
	ld -N -e start -Ttext 0x7c00 --oformat binary -o $@ loader.o

diskimage.bin: loader.bin kernel.bin
	cat loader.bin kernel.bin > diskimage.bin

clean:
	rm -f *.o *.d *.bin intr-stubs.S


