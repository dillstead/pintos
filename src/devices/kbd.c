#include "kbd.h"
#include "debug.h"
#include "interrupt.h"
#include "io.h"
#include "lib.h"

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
