#ifndef USERPROG_TSS_H
#define USERPROG_TSS_H

#include <stdint.h>

struct tss;
void tss_init (void);
struct tss *tss_get (void);
void tss_set_esp0 (uint8_t *);

#endif /* userprog/tss.h */
