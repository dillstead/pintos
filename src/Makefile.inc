# -*- makefile -*-

SHELL = /bin/sh
VPATH := $(TOP_SRCDIR)/threads
VPATH := $(VPATH):$(TOP_SRCDIR)/devices
VPATH := $(VPATH):$(TOP_SRCDIR)/lib
VPATH := $(VPATH):$(TOP_SRCDIR)/filesys
VPATH := $(VPATH):$(TOP_SRCDIR)/userprog

DEFINES += -DCNACHOS86
WARNINGS = -Wall -W -Wstrict-prototypes -Wmissing-prototypes
CFLAGS = -g -O3 -MMD $(WARNINGS) $(INCLUDES) $(DEFINES)
ASFLAGS = -Wa,--gstabs+ $(INCLUDES) $(DEFINES)

# Core kernel.
THREADS_SRC  = start.S		# Must be linked first.
THREADS_SRC += init.c		# Start-up code.
THREADS_SRC += thread.c		# Thread management core.
THREADS_SRC += switch.S		# Thread switch routine.
THREADS_SRC += interrupt.c	# Interrupt core.
THREADS_SRC += intr-stubs.S	# Interrupt stubs.
THREADS_SRC += synch.c		# Synchronization.
THREADS_SRC += paging.c		# Page tables.
THREADS_SRC += palloc.c		# Page allocator.
THREADS_SRC += malloc.c		# Subpage allocator.

# Device driver code.
DEVICES_SRC  = timer.c		# Timer device.
DEVICES_SRC += kbd.c		# Keyboard device.
DEVICES_SRC += vga.c		# Video device.
DEVICES_SRC += serial.c		# Serial port device.
DEVICES_SRC += disk.c		# IDE disk device.

# Library code.
LIB_SRC  = debug.c		# Debug helpers.
LIB_SRC += lib.c		# Standard C library.
LIB_SRC += random.c		# Pseudo-random numbers.
LIB_SRC += list.c		# Doubly-linked lists.
LIB_SRC += bitmap.c		# Bitmaps.
LIB_SRC += backdoor.c		# Backdoor IPC.

# Filesystem code.
FILESYS_SRC  = filesys.c	# Filesystem core.
FILESYS_SRC += file.c		# Individual files.
FILESYS_SRC += filesys-stub.c	# Stub helper code.

# User process code.
USERPROG_SRC  = addrspace.c	# Address spaces.

# Objects.
OBJECTS = $(patsubst %.c,%.o,$(patsubst %.S,%.o,$(SOURCES)))

all: diskimage.bin

intr-stubs.S: $(TOP_SRCDIR)/threads/intr-stubs.pl
	$< > $@

kernel.o: $(OBJECTS)
	ld -T $(TOP_SRCDIR)/threads/kernel.lds -o $@ $^ \
		`$(CC) -print-libgcc-file-name`

kernel.bin: kernel.o
	objcopy -O binary -R .note -R .comment -S $< $@.tmp
	$(TOP_SRCDIR)/pad 4096 < $@.tmp > $@

loader.bin: loader.S kernel.bin
	gcc -c $< -DKERNEL_LOAD_PAGES=`perl -e 'print +(-s "kernel.bin") / 4096;'`
	ld -N -e start -Ttext 0x7c00 --oformat binary -o $@ loader.o

diskimage.bin: loader.bin kernel.bin
	cat loader.bin kernel.bin > diskimage.bin

clean:
	rm -f *.o *.d *.bin kernel.bin.data kernel.bin.pad intr-stubs.S

-include *.d
