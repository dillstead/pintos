#include <stdio.h>
#include <syscall.h>
#include <syscall-nr.h>

static void vprintf_helper (char, void *);

/* Auxiliary data for vprintf_helper(). */
struct vprintf_aux 
  {
    char buf[64];       /* Character buffer. */
    char *p;            /* Current position in buffer. */
    int char_cnt;       /* Total characters written so far. */
  };

/* The standard vprintf() function,
   which is like printf() but uses a va_list.
   Writes its output to the STDOUT_FILENO handle. */
int
vprintf (const char *format, va_list args) 
{
  struct vprintf_aux aux;
  aux.p = aux.buf;
  aux.char_cnt = 0;
  __vprintf (format, args, vprintf_helper, &aux);
  if (aux.p > aux.buf)
    write (STDOUT_FILENO, aux.buf, aux.p - aux.buf);
  return aux.char_cnt;
}

/* Helper function for vprintf(). */
static void
vprintf_helper (char c, void *aux_) 
{
  struct vprintf_aux *aux = aux_;
  *aux->p++ = c;
  if (aux->p >= aux->buf + sizeof aux->buf)
    {
      write (STDOUT_FILENO, aux->buf, aux->p - aux->buf);
      aux->p = aux->buf;
    }
  aux->char_cnt++;
}

/* Writes C to the console. */
int
putchar (int c) 
{
  char c2 = c;
  write (STDOUT_FILENO, &c2, 1);
  return c;
}
