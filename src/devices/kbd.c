#include "kbd.h"
#include "lib/debug.h"
#include "lib/lib.h"
#include "threads/interrupt.h"
#include "threads/io.h"

static void
irq21_keyboard (struct intr_frame *args UNUSED) 
{
  printk ("Keyboard!\n");
  inb (0x60);
}

void
kbd_init (void) 
{
  intr_register (0x21, 0, INTR_OFF, irq21_keyboard, "8042 Keyboard");
}
