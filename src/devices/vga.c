#include "vga.h"
#include <stdint.h>
#include <stddef.h>
#include "io.h"
#include "lib.h"
#include "mmu.h"

static size_t vga_cols, vga_rows;
static size_t vga_x, vga_y;
static uint16_t *vga_base;

void
vga_init (void)
{
  vga_cols = 80;
  vga_rows = 25;
  vga_base = (uint16_t *) ptov (0xb8000);
  vga_cls ();
}

void
vga_cls (void)
{
  size_t i;
  for (i = 0; i < vga_cols * vga_rows; i++)
    vga_base[i] = 0x0700;
  vga_x = vga_y = 0;
}

static void
vga_newline (void)
{
  vga_x = 0;
  vga_y++;
  if (vga_y >= vga_rows)
    {
      vga_y = vga_rows - 1;
      memmove (vga_base, vga_base + vga_cols,
               vga_cols * (vga_rows - 1) * 2);
      memset (vga_base + vga_cols * (vga_rows - 1),
              0, vga_cols * 2);
    }
}

static void
move_cursor (void) 
{
  uint16_t cp = (vga_x + vga_cols * vga_y);
  outw (0x3d4, 0x0e | (cp & 0xff00));
  outw (0x3d4, 0x0f | (cp << 8));
}

void
vga_putc (int c)
{
  switch (c) 
    {
    case '\n':
      vga_newline ();
      break;

    case '\f':
      vga_cls ();
      break;

    case '\b':
      if (vga_x > 0)
        vga_x--;
      break;
      
    case '\r':
      vga_x = 0;
      break;

    case '\t':
      vga_x = (vga_x + 8) / 8 * 8;
      if (vga_x >= vga_cols)
        vga_newline ();
      break;
      
    default:
      vga_base[vga_x + vga_y * vga_cols] = 0x0700 | c;
      vga_x++;
      if (vga_x >= vga_cols)
        vga_newline ();
      break;
    }

  move_cursor ();
}
