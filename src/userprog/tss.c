#include "tss.h"
#include <stddef.h>
#include "debug.h"
#include "gdt.h"
#include "mmu.h"
#include "palloc.h"

struct tss
  {
    uint16_t back_link, :16;
    void *esp0;
    uint16_t ss0, :16;
    void *esp1;
    uint16_t ss1, :16;
    void *esp2;
    uint16_t ss2, :16;
    uint32_t cr3;
    void (*eip) (void);
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

static struct tss *tss;

void
tss_init (void) 
{
  /* Our TSS is never used in a call gate or task gate, so only a
     few fields of it are ever referenced, and those are the only
     ones we initialize. */
  tss = palloc_get (PAL_ASSERT | PAL_ZERO);
  tss->esp0 = ptov(0x20000);
  tss->ss0 = SEL_KDSEG;
  tss->bitmap = 0xdfff;
}

struct tss *
tss_get (void) 
{
  ASSERT (tss != NULL);
  return tss;
}

void
tss_set_esp0 (uint8_t *esp0) 
{
  ASSERT (tss != NULL);
  tss->esp0 = esp0;
}
