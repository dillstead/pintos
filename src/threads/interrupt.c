#include "interrupt.h"
#include <stdint.h>
#include "debug.h"
#include "io.h"
#include "lib.h"
#include "mmu.h"
#include "thread.h"
#include "timer.h"

enum if_level
intr_get_level (void) 
{
  uint32_t flags;
  
  asm ("pushfl; popl %0" : "=g" (flags));

  return flags & (1 << 9) ? IF_ON : IF_OFF;
}

enum if_level
intr_set_level (enum if_level level) 
{
  enum if_level old_level = intr_get_level ();
  if (level == IF_ON)
    intr_enable ();
  else
    intr_disable ();
  return old_level;
}

enum if_level
intr_enable (void) 
{
  enum if_level old_level = intr_get_level ();
  asm volatile ("sti");
  return old_level;
}

enum if_level
intr_disable (void) 
{
  enum if_level old_level = intr_get_level ();
  asm volatile ("cli");
  return old_level;
}

static void
pic_init (void)
{
  /* Every PC has two 8259A Programmable Interrupt Controller
     (PIC) chips.  One is a "master" accessible at ports 0x20 and
     0x21.  The other is a "slave" cascaded onto the master's IRQ
     2 line and accessible at ports 0xa0 and 0xa1.  Accesses to
     port 0x20 set the A0 line to 0 and accesses to 0x21 set the
     A1 line to 1.  The situation is similar for the slave PIC.
     Refer to the 8259A datasheet for details.

     By default, interrupts 0...15 delivered by the PICs will go
     to interrupt vectors 0...15.  Unfortunately, those vectors
     are also used for CPU traps and exceptions.  We reprogram
     the PICs so that interrupts 0...15 are delivered to
     interrupt vectors 32...47 instead. */

  /* Mask all interrupts on both PICs. */
  outb (0x21, 0xff);
  outb (0xa1, 0xff);

  /* Initialize master. */
  outb (0x20, 0x11); /* ICW1: single mode, edge triggered, expect ICW4. */
  outb (0x21, 0x20); /* ICW2: line IR0...7 -> irq 0x20...0x27. */
  outb (0x21, 0x04); /* ICW3: slave PIC on line IR2. */
  outb (0x21, 0x01); /* ICW4: 8086 mode, normal EOI, non-buffered. */

  /* Initialize slave. */
  outb (0xa0, 0x11); /* ICW1: single mode, edge triggered, expect ICW4. */
  outb (0xa1, 0x28); /* ICW2: line IR0...7 -> irq 0x28...0x2f. */
  outb (0xa1, 0x02); /* ICW3: slave ID is 2. */
  outb (0xa1, 0x01); /* ICW4: 8086 mode, normal EOI, non-buffered. */

  /* Unmask all interrupts. */
  outb (0x21, 0x00);
  outb (0xa1, 0x00);
}

/* Sends an end-of-interrupt signal to the PIC for the given IRQ.
   If we don't acknowledge the IRQ, we'll never get it again, so
   this is important.  */
static void
pic_eoi (void) 
{
  /* FIXME?  The Linux code is much more complicated. */
  outb (0x20, 0x20);
}

uint64_t idt[256];

extern void (*intr_stubs[256]) (void);

intr_handler_func *intr_handlers[256];

void intr_handler (struct intr_args *args);

bool intr_in_progress;
bool yield_on_return;

void
intr_handler (struct intr_args *args) 
{
  bool external;
  
  yield_on_return = false;

  external = args->vec_no >= 0x20 && args->vec_no < 0x30;
  if (external) 
    {
      ASSERT (intr_get_level () == IF_OFF);
      ASSERT (!intr_context ());
      intr_in_progress = true;
    }

  intr_handlers[args->vec_no] (args);

  if (external) 
    {
      ASSERT (intr_get_level () == IF_OFF);
      ASSERT (intr_context ());
      intr_in_progress = false;
      pic_eoi (); 
    }

  if (yield_on_return) 
    {
      printk (".");
      thread_yield (); 
    }
}

bool
intr_context (void) 
{
  return intr_in_progress;
}

void
intr_yield_on_return (void) 
{
  yield_on_return = true;
}

/* Handles interrupts we don't know about. */
intr_handler_func intr_unexpected;

/* Handlers for CPU exceptions. */
intr_handler_func excp00_divide_error;
intr_handler_func excp01_debug;
intr_handler_func excp02_nmi;
intr_handler_func excp03_breakpoint;
intr_handler_func excp04_overflow;
intr_handler_func excp05_bound;
intr_handler_func excp06_invalid_opcode;
intr_handler_func excp07_device_not_available;
intr_handler_func excp08_double_fault;
intr_handler_func excp09_coprocessor_overrun;
intr_handler_func excp0a_invalid_tss;
intr_handler_func excp0b_segment_not_present;
intr_handler_func excp0c_stack_fault;
intr_handler_func excp0d_general_protection;
intr_handler_func excp0e_page_fault;
intr_handler_func excp10_fp_error;
intr_handler_func excp11_alignment;
intr_handler_func excp12_machine_check;
intr_handler_func excp13_simd_error;

static uint64_t
make_intr_gate (void (*target) (void),
                int dpl)
{
  uint32_t offset = (uint32_t) target;
  uint32_t e0 = ((offset & 0xffff)            /* Offset 15:0. */
                 | (SEL_KCSEG << 16));        /* Target code segment. */
  uint32_t e1 = ((offset & 0xffff0000)        /* Offset 31:16. */
                 | (1 << 15)                  /* Present. */
                 | (dpl << 13)                /* Descriptor privilege. */
                 | (SYS_SYSTEM << 12)         /* System. */
                 | (TYPE_INT_32 << 8));       /* 32-bit interrupt gate. */
  return e0 | ((uint64_t) e1 << 32);
}

static uint64_t
make_trap_gate (void (*target) (void),
                int dpl)
{
  return make_intr_gate (target, dpl) | (1 << 8);
}

/* We don't support nested interrupts generated by external
   hardware, so these interrupts (vec_no 0x20...0x2f) should
   specify IF_OFF for LEVEL.  Otherwise a timer interrupt could
   cause a task switch during interrupt handling.  Most other
   interrupts can and should be handled with interrupts
   enabled. */
void
intr_register (uint8_t vec_no, int dpl, enum if_level level,
               intr_handler_func *handler) 
{
  if (level == IF_ON)
    idt[vec_no] = make_trap_gate (intr_stubs[vec_no], dpl);
  else
    idt[vec_no] = make_intr_gate (intr_stubs[vec_no], dpl);
  intr_handlers[vec_no] = handler;
}

void
intr_init (void)
{
  uint64_t idtr_operand;
  int i;

  pic_init ();

  /* Install default handlers. */
  for (i = 0; i < 256; i++)
    intr_register (i, 0, IF_OFF, intr_unexpected);

  /* Most exceptions require ring 0.
     Exceptions 3, 4, and 5 can be caused by ring 3 directly.

     Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved.
  */
#if 0
  intr_register (0x00, 0, IF_ON, excp00_divide_error);
  intr_register (0x01, 0, IF_ON, excp01_debug);
  intr_register (0x02, 0, IF_ON, excp02_nmi);
  intr_register (0x03, 3, IF_ON, excp03_breakpoint);
  intr_register (0x04, 3, IF_ON, excp04_overflow);
  intr_register (0x05, 3, IF_ON, excp05_bound);
  intr_register (0x06, 0, IF_ON, excp06_invalid_opcode);
  intr_register (0x07, 0, IF_ON, excp07_device_not_available);
  intr_register (0x08, 0, IF_ON, excp08_double_fault);
  intr_register (0x09, 0, IF_ON, excp09_coprocessor_overrun);
  intr_register (0x0a, 0, IF_ON, excp0a_invalid_tss);
  intr_register (0x0b, 0, IF_ON, excp0b_segment_not_present);
  intr_register (0x0c, 0, IF_ON, excp0c_stack_fault);
  intr_register (0x0d, 0, IF_ON, excp0d_general_protection);
  intr_register (0x0e, 0, IF_OFF, excp0e_page_fault);
  intr_register (0x10, 0, IF_ON, excp10_fp_error);
  intr_register (0x11, 0, IF_ON, excp11_alignment);
  intr_register (0x12, 0, IF_ON, excp12_machine_check);
  intr_register (0x13, 0, IF_ON, excp13_simd_error);
#endif

  idtr_operand = make_dtr_operand (sizeof idt - 1, idt);
  asm volatile ("lidt %0" :: "m" (idtr_operand));
}

void
intr_unexpected (struct intr_args *regs)
{
  uint32_t cr2;
  asm ("movl %%cr2, %0" : "=r" (cr2));
  printk ("Unexpected interrupt 0x%02x, error code %08x, cr2=%08x, eip=%08x\n",
          regs->vec_no, regs->error_code, cr2, regs->eip);
  for (;;);
}
