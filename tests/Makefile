TESTS = threads p1-1 p1-2 p1-3 list stdlib userprog p2 vm filesys

PATH := $(shell pwd)/../src/utils:$(PATH)

all:
	@echo "This Makefile has only \`check' targets."

check:
	$(MAKE) -C .. distclean
	for d in $(TESTS); do $(MAKE) $$d || exit 1; done
	@echo All tests passed.

clean:
	rm -rf $(TESTS)

define prep-threads-grading
endef

define prep-userprog-grading
$(MAKE) -C ../src/userprog -s
$(MAKE) -C ../grading/userprog -s
endef

define prep-vm-grading
$(MAKE) -C ../src/userprog -s
$(MAKE) -C ../grading/vm -s
endef

define prep-filesys-grading
$(MAKE) -C ../grading/filesys -s
endef

define prep-grading
$(prep-$(PROJECT)-grading)
endef

define mk-sandbox
rm -rf $@ && mkdir -p $@/pintos && cp -R ../src $@/pintos
endef

define run-tests
cd $@ && ../../grading/$(PROJECT)/run-tests
endef

define clean
rm -rf $@/pintos
endef

define apply-patch
(cd $@/pintos && patch -p0 -s) <
endef

threads: PROJECT = threads
threads::
	$(mk-sandbox)
	$(run-tests) -d alarm-single
	$(clean)

p1-1: PROJECT = threads
p1-1::
	$(mk-sandbox)
	$(apply-patch) ../solutions/p1-1.patch
	$(run-tests) -d alarm.*
	$(clean)

p1-2: PROJECT = threads
p1-2::
	$(mk-sandbox)
	$(apply-patch) ../solutions/p1-2.patch
	$(run-tests) -d join.*
	$(clean)

p1-3: PROJECT = threads
p1-3::
	$(mk-sandbox)
	$(apply-patch) ../solutions/p1-3.patch
	$(run-tests) -d priority.*
	$(clean)

list: PROJECT = threads
list stdlib::
	$(mk-sandbox)
	cp ../src/tests/threads/$@.c $@/pintos/src/threads/test.c
	$(MAKE) -C $@/pintos/src/threads -s
	-(cd $@/pintos/src/threads/build && pintos -v run -q) | tee $@/output
	grep -q '$@: PASS' $@/output
	$(clean)

userprog: PROJECT = userprog
userprog::
	$(prep-grading)
	$(mk-sandbox)
	$(apply-patch) ../solutions/p1-2.patch
	echo '#define THREAD_JOIN_IMPLEMENTED 1' > $@/pintos/src/constants.h
	$(run-tests) null
	$(clean)

p2: PROJECT = userprog
p2::
	$(prep-grading)
	$(mk-sandbox)
	$(apply-patch) ../solutions/p2.patch
	$(run-tests)
	$(clean)

vm: PROJECT = vm
vm::
	$(prep-grading)
	$(mk-sandbox)
	$(MAKE) -C $@/pintos/src/vm -s
	$(clean)

filesys: PROJECT = filesys
filesys::
	$(prep-grading)
	$(mk-sandbox)
	$(MAKE) -C $@/pintos/src/filesys -s
	$(clean)
