#ifndef HEADER_INTR_STUBS_H
#define HEADER_INTR_STUBS_H

extern void (*const intr_stubs[256]) (void);

void intr_entry (void);
void intr_exit (void);

#endif /* intr-stubs.h */
