#ifndef HEADER_GDT_H
#define HEADER_GDT_H 1

/* Segment selectors. */
#define SEL_NULL        0x00    /* Null selector. */
#define SEL_KCSEG       0x08    /* Kernel code selector. */
#define SEL_KDSEG       0x10    /* Kernel data selector. */
#define SEL_UCSEG       0x1B    /* User code selector. */
#define SEL_UDSEG       0x23    /* User data selector. */
#define SEL_TSS         0x28    /* Task-state segment. */
#define SEL_CNT         6       /* Number of segments. */

#ifndef __ASSEMBLER__
#include <stdint.h>

static inline uint64_t
make_dtr_operand (uint16_t limit, void *base)
{
  return limit | ((uint64_t) (uint32_t) base << 16);
}

void gdt_init (void);
#endif

#endif /* gdt.h */
