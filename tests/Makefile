TESTS = threads p1 userprog p2 vm p3 filesys p4

PATH := $(shell pwd)/../src/utils:$(PATH)

all:
	@echo "This Makefile has only \`check' targets."

check:
	$(MAKE) -C .. distclean
	for d in $(TESTS); do $(MAKE) $$d || exit 1; done
	@echo All tests passed.

clean:
	rm -rf $(TESTS)

SUBMAKEFLAGS = -s

define mk-sandbox
rm -rf $@ && mkdir $@ && cp -R ../src $@/src
cd $@/src && $(MAKE) clean $(SUBMAKEFLAGS)
endef

define run-tests
cd $@/src/$(PROJECT) && make check && make grade
endef

define compile
cd $@/src/$(PROJECT) && make
endef

define clean
rm -rf $@
endef

define apply-patch
(cd $@ && patch -p0)
endef

PROJECT = $@

threads::
	$(mk-sandbox)
	$(compile)
	$(run-tests) TESTS=tests/threads/alarm-single
	$(clean)

userprog vm filesys::
	$(mk-sandbox)
	$(compile)
	$(clean)

p1: PROJECT = threads
p1::
	$(mk-sandbox)
	$(apply-patch) < ../solutions/p1.patch
	$(run-tests)
	$(clean)

p2: PROJECT = userprog
p2::
	$(mk-sandbox)
	$(apply-patch) < ../solutions/p2.patch
	$(run-tests)
	$(clean)

p3: PROJECT = vm
p3::
	$(mk-sandbox)
	$(apply-patch) < ../solutions/p3.patch
	$(run-tests)
	$(clean)

p4: PROJECT = filesys
p4::
	$(mk-sandbox)
	$(apply-patch) < ../solutions/p4.patch
	$(run-tests)
	$(clean)
