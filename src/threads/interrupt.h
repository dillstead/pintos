#ifndef HEADER_INTERRUPT_H
#define HEADER_INTERRUPT_H 1

#include <stdbool.h>
#include <stdint.h>

enum if_level 
  {
    IF_OFF,             /* Interrupts disabled. */
    IF_ON               /* Interrupts enabled. */
  };

enum if_level intr_get_level (void);
enum if_level intr_set_level (enum if_level);
enum if_level intr_enable (void);
enum if_level intr_disable (void);

struct intr_args
  {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint16_t es, :16;
    uint16_t ds, :16;
    uint32_t vec_no;
    uint32_t error_code;
    uint32_t eip;
  };

typedef void intr_handler_func (struct intr_args *);

void intr_init (void);
void intr_register (uint8_t vec, int dpl, enum if_level, intr_handler_func *);
bool intr_context (void);
void intr_yield_on_return (void);

#endif /* interrupt.h */
