#include "devices/kbd.h"
#include <debug.h>
#include <stdio.h>
#include "threads/interrupt.h"
#include "threads/io.h"

static void
irq21_keyboard (struct intr_frame *args UNUSED) 
{
  printf ("Keyboard!\n");
  inb (0x60);
}

void
kbd_init (void) 
{
  intr_register (0x21, 0, INTR_OFF, irq21_keyboard, "8042 Keyboard");
}
