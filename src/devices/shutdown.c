#include "devices/shutdown.h"
#include <console.h>
#include <stdio.h>
#include "devices/kbd.h"
#include "devices/serial.h"
#include "devices/timer.h"
#include "threads/io.h"
#include "threads/thread.h"
#ifdef USERPROG
#include "userprog/exception.h"
#endif
#ifdef FILESYS
#include "devices/disk.h"
#include "filesys/filesys.h"
#endif

/* Keyboard control register port. */
#define CONTROL_REG 0x64

static void print_stats (void);

/* Reboots the machine via the keyboard controller. */
void
shutdown_reboot (void)
{
  int i;

  printf ("Rebooting...\n");

    /* See [kbd] for details on how to program the keyboard
     * controller. */
  for (i = 0; i < 100; i++)
    {
      int j;

      /* Poll keyboard controller's status byte until
       * 'input buffer empty' is reported. */
      for (j = 0; j < 0x10000; j++)
        {
          if ((inb (CONTROL_REG) & 0x02) == 0)
            break;
          timer_udelay (2);
        }

      timer_udelay (50);

      /* Pulse bit 0 of the output port P2 of the keyboard controller.
       * This will reset the CPU. */
      outb (CONTROL_REG, 0xfe);
      timer_udelay (50);
    }
}

/* Powers down the machine we're running on,
   as long as we're running on Bochs or QEMU. */
void
shutdown_power_off (void)
{
  const char s[] = "Shutdown";
  const char *p;

#ifdef FILESYS
  filesys_done ();
#endif

  print_stats ();

  printf ("Powering off...\n");
  serial_flush ();

  /* This is a special power-off sequence supported by Bochs and
     QEMU, but not by physical hardware. */
  for (p = s; *p != '\0'; p++)
    outb (0x8900, *p);

  /* This will power off a VMware VM if "gui.exitOnCLIHLT = TRUE"
     is set in its configuration file.  (The "pintos" script does
     that automatically.)  */
  asm volatile ("cli; hlt" : : : "memory");

  /* None of those worked. */
  printf ("still running...\n");
  for (;;);
}

/* Print statistics about Pintos execution. */
static void
print_stats (void)
{
  timer_print_stats ();
  thread_print_stats ();
#ifdef FILESYS
  disk_print_stats ();
#endif
  console_print_stats ();
  kbd_print_stats ();
#ifdef USERPROG
  exception_print_stats ();
#endif
}
