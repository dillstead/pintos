#ifndef HEADER_TSS_H
#define HEADER_TSS_H

#include <stdint.h>

struct tss;
void tss_init (void);
struct tss *tss_get (void);
void tss_set_esp0 (uint8_t *);

#endif /* tss.h */
