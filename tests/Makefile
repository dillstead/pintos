TESTS = threads p1-1 p1-2 list stdlib stdio userprog p2 vm filesys

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

define prep-threads-grading
endef

define prep-userprog-grading
$(MAKE) -C ../src/userprog $(SUBMAKEFLAGS)
$(MAKE) -C ../grading/userprog $(SUBMAKEFLAGS)
endef

define prep-vm-grading
$(MAKE) -C ../src/userprog $(SUBMAKEFLAGS)
$(MAKE) -C ../grading/vm $(SUBMAKEFLAGS)
endef

define prep-filesys-grading
$(MAKE) -C ../grading/filesys $(SUBMAKEFLAGS)
endef

define prep-grading
$(prep-$(PROJECT)-grading)
endef

define mk-sandbox
rm -rf $@ && mkdir -p $@/pintos && cp -R ../src $@/pintos
cd $@/pintos/src && $(MAKE) clean $(SUBMAKEFLAGS)
endef

define run-tests
cd $@ && ../../grading/$(PROJECT)/run-tests
endef

define clean
rm -rf $@/pintos
endef

define apply-patch
(cd $@/pintos && patch -p0)
endef

threads: PROJECT = threads
threads::
	$(mk-sandbox)
	$(run-tests) -d alarm-single
	$(clean)

p1-1: PROJECT = threads
p1-1::
	$(mk-sandbox)
	$(apply-patch) < ../solutions/p1-1.patch
	$(run-tests) -d alarm.*
	$(clean)

p1-2: PROJECT = threads
p1-2::
	$(mk-sandbox)
	$(apply-patch) < ../solutions/p1-2.patch
	$(run-tests) -d priority.*
	$(clean)

list: PROJECT = threads
list stdlib stdio::
	$(mk-sandbox)
	cp ../src/tests/threads/$@.c $@/pintos/src/threads/test.c
	$(MAKE) -C $@/pintos/src/threads $(SUBMAKEFLAGS)
	-(cd $@/pintos/src/threads/build && pintos -v run -q) | tee $@/output
	grep -q '$@: PASS' $@/output
	$(clean)

userprog: PROJECT = userprog
userprog::
	$(prep-grading) null.dsk
	$(mk-sandbox)
	$(apply-patch) < ../solutions/p2-null.patch
	$(run-tests) null
	$(clean)

p2: PROJECT = userprog
p2::
	$(prep-grading)
	$(mk-sandbox)
	$(apply-patch) < ../solutions/p2.patch
	$(run-tests)
	$(clean)

vm: PROJECT = vm
vm::
	$(prep-grading)
	$(mk-sandbox)
	$(MAKE) -C $@/pintos/src/vm $(SUBMAKEFLAGS)
	$(clean)

filesys: PROJECT = filesys
filesys::
	$(prep-grading)
	$(mk-sandbox)
	$(MAKE) -C $@/pintos/src/filesys $(SUBMAKEFLAGS)
	$(clean)
