#ifndef HEADER_INTR_STUBS_H
#define HEADER_INTR_STUBS_H

/* Interrupt stubs.

   These are little snippets of code in intr-stubs.S, one for
   each of the 256 possible x86 interrupts.  They just push the
   interrupt vector number on the stack (and, for interrupts that
   don't have an error code, a fake error code), then jump to
   intr_entry().

   This array points to each of the interrupt stub entry points
   so that intr_init() can easily find them. */
typedef void intr_stub_func (void);
extern intr_stub_func *intr_stubs[256];

/* Interrupt return path. */
void intr_exit (void);

#endif /* intr-stubs.h */
