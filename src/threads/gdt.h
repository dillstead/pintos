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

struct tss
  {
    uint16_t back_link, :16;
    uint32_t esp0;
    uint16_t ss0, :16;
    uint32_t esp1;
    uint16_t ss1, :16;
    uint32_t esp2;
    uint16_t ss2, :16;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint16_t es, :16;
    uint16_t cs, :16;
    uint16_t ss, :16;
    uint16_t ds, :16;
    uint16_t fs, :16;
    uint16_t gs, :16;
    uint16_t ldt, :16;
    uint16_t trace, bitmap;
  };


static inline uint64_t
make_dtr_operand (uint16_t limit, void *base)
{
  return limit | ((uint64_t) (uint32_t) base << 16);
}

extern struct tss *tss;

void gdt_init (void);
#endif

#endif /* gdt.h */
