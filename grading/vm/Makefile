include Make.progs

SRCDIR = ../../src

DISKS = $(patsubst %,%.dsk,$(TESTS))

disks: $(DISKS)

mmap-close.dsk mmap-exit.dsk mmap-read.dsk mmap-unmap.dsk: sample.txt
mmap-overlap.dsk: zeros

zeros:
	dd if=/dev/zero of=$@ bs=1024 count=6

%.dsk: %
	./prep-disk --fs-disk=$@ $^

clean::
	rm -f $(DISKS)

include $(SRCDIR)/Makefile.userprog

CFLAGS += -Werror
