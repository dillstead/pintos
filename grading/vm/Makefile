include Make.progs

SRCDIR = ../../src

DISKS = $(patsubst %,%.dsk,$(TESTS))

disks: $(DISKS)

mmap-close.dsk mmap-read.dsk mmap-unmap.dsk mmap-twice.dsk: sample.txt
page-parallel.dsk: child-linear
page-merge-seq.dsk page-merge-par.dsk: child-sort
mmap-overlap.dsk: zeros
mmap-exit.dsk: child-mm-wrt

zeros:
	dd if=/dev/zero of=$@ bs=1024 count=6

%.dsk: %
	./prep-disk --fs-disk=$@ $^

clean::
	rm -f $(DISKS)

include $(SRCDIR)/Makefile.userprog

CFLAGS += -Werror
