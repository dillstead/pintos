#include "devices/kbd.h"
#include <ctype.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "devices/intq.h"
#include "threads/interrupt.h"
#include "threads/io.h"

/* Keyboard data register port. */
#define DATA_REG 0x60

/* Shift state bits. */
#define LSHIFT  0x01            /* Left Shift. */
#define RSHIFT  0x02            /* Right Shift. */
#define LALT    0x04            /* Left Alt. */
#define RALT    0x08            /* Right Alt. */
#define LCTRL   0x10            /* Left Ctrl. */
#define RCTRL   0x20            /* Right Ctrl. */
#define CAPS    0x40            /* Caps Lock. */

/* Current shift state. */
static unsigned shift_state;

/* Keyboard buffer. */
static struct intq buffer;

/* Number of keys pressed. */
static int64_t key_cnt;

static intr_handler_func keyboard_interrupt;

/* Initializes the keyboard. */
void
kbd_init (void) 
{
  intq_init (&buffer, "keyboard");
  intr_register (0x21, 0, INTR_OFF, keyboard_interrupt, "8042 Keyboard");
}

/* Retrieves a key from the keyboard buffer.
   If the buffer is empty, waits for a key to be pressed. */
uint8_t
kbd_getc (void) 
{
  enum intr_level old_level;
  uint8_t key;

  old_level = intr_disable ();
  key = intq_getc (&buffer);
  intr_set_level (old_level);
  
  return key;
}

/* Prints keyboard statistics. */
void
kbd_print_stats (void) 
{
  printf ("Keyboard: %lld keys pressed\n", key_cnt);
}

/* Maps a set of contiguous scancodes into characters. */
struct keymap
  {
    uint8_t first_scancode;     /* First scancode. */
    const char *chars;          /* chars[0] has scancode first_scancode,
                                   chars[1] has scancode first_scancode + 1,
                                   and so on to the end of the string. */
  };
  
/* Keys that produce the same characters regardless of whether
   the Shift keys are down.  Case of letters is an exception
   that we handle elsewhere.  */
static const struct keymap invariant_keymap[] = 
  {
    {0x01, "\033"},
    {0x0e, "\b"},
    {0x0f, "\tQWERTYUIOP"},
    {0x1c, "\n"},
    {0x1e, "ASDFGHJKL"},
    {0x2c, "ZXCVBNM"},
    {0x37, "*"},
    {0x39, " "},
    {0, NULL},
  };

/* Characters for keys pressed without Shift, for those keys
   where it matters. */
static const struct keymap unshifted_keymap[] = 
  {
    {0x02, "1234567890-="},
    {0x1a, "[]"},
    {0x27, ";'`"},
    {0x2b, "\\"},
    {0x33, ",./"},
    {0, NULL},
  };
  
/* Characters for keys pressed with Shift, for those keys where
   it matters. */
static const struct keymap shifted_keymap[] = 
  {
    {0x02, "!@#$%^&*()_+"},
    {0x1a, "{}"},
    {0x27, ":\"~"},
    {0x2b, "|"},
    {0x33, "<>?"},
    {0, NULL},
  };

static bool map_key (const struct keymap[], unsigned scancode, uint8_t *);

static void
keyboard_interrupt (struct intr_frame *args UNUSED) 
{
  /* Status of shift keys. */
  bool shift = (shift_state & (LSHIFT | RSHIFT)) != 0;
  bool alt = (shift_state & (LALT | RALT)) != 0;
  bool ctrl = (shift_state & (LCTRL | RCTRL)) != 0;
  bool caps = (shift_state & CAPS) != 0;

  /* Keyboard scancode. */
  unsigned code;

  /* False if key pressed, true if key released. */
  bool release;

  /* Character that corresponds to `code'. */
  uint8_t c;

  /* Read scancode, including second byte if prefix code. */
  code = inb (DATA_REG);
  if (code == 0xe0)
    code = (code << 8) | inb (DATA_REG);

  /* Bit 0x80 distinguishes key press from key release
     (even if there's a prefix). */
  release = (code & 0x80) != 0;
  code &= ~0x80u;

  /* Interpret key. */
  if (code == 0x3a) 
    {
      /* Caps Lock. */
      if (!release)
        shift_state ^= CAPS; 
    }
  else if (map_key (invariant_keymap, code, &c)
           || (!shift && map_key (unshifted_keymap, code, &c))
           || (shift && map_key (shifted_keymap, code, &c)))
    {
      /* Ordinary character. */
      if (!release) 
        {
          /* Handle Ctrl, Shift.
             Note that Ctrl overrides Shift. */
          if (ctrl && c >= 0x40 && c < 0x60) 
            {
              /* A is 0x41, Ctrl+A is 0x01, etc. */
              c -= 0x40; 
            }
          else if (shift == caps)
            c = tolower (c);

          /* Handle Alt by setting the high bit.
             This 0x80 is unrelated to the one used to
             distinguish key press from key release. */
          if (alt)
            c += 0x80;

          /* Append to keyboard buffer. */
          if (!intq_full (&buffer)) 
            {
              key_cnt++;
              intq_putc (&buffer, c); 
            }
        }
    }
  else
    {
      /* Table of shift keys.
         Maps a keycode into a shift_state bit. */
      static const unsigned shift_keys[][2] = 
        {
          {  0x2a, LSHIFT},
          {  0x36, RSHIFT},
          {  0x38, LALT},
          {0xe038, RALT},
          {  0x1d, LCTRL},
          {0xe01d, RCTRL},
          {0, 0},
        };
  
      const unsigned (*key)[2];

      /* Scan the table. */
      for (key = shift_keys; (*key)[0] != 0; key++) 
        if ((*key)[0] == code)
          {
            if (release)
              shift_state &= ~(*key)[1];
            else
              shift_state |= (*key)[1];
            break;
          }
    }
}

/* Scans the array of keymaps K for SCANCODE.
   If found, sets *C to the corresponding character and returns
   true.
   If not found, returns false and C is ignored. */
static bool
map_key (const struct keymap k[], unsigned scancode, uint8_t *c) 
{
  for (; k->first_scancode != 0; k++)
    if (scancode >= k->first_scancode
        && scancode < k->first_scancode + strlen (k->chars)) 
      {
        *c = k->chars[scancode - k->first_scancode];
        return true; 
      }

  return false;
}
