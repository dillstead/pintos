#include "malloc.h"
#include <stdint.h>
#include "debug.h"
#include "lib.h"
#include "mmu.h"
#include "palloc.h"

struct desc
  {
    size_t slot_size;           /* Size of each element in bytes. */
    struct slot *free_list;     /* Unused slots. */
    struct arena *arenas;       /* Arenas. */
  };

struct arena 
  {
    struct desc *desc;          /* Owning descriptor. */
    struct arena *prev, *next;  /* Doubly linked list of arenas. */
  };

struct slot 
  {
    struct slot *next;          /* Singly linked list of slots. */
  };

struct desc descs[16];
size_t desc_cnt;

void
malloc_init (void) 
{
  size_t slot_size;

  for (slot_size = 16; slot_size < PGSIZE; slot_size *= 2)
    {
      struct desc *d = &descs[desc_cnt++];
      ASSERT (desc_cnt <= sizeof descs / sizeof *descs);
      d->slot_size = slot_size;
      d->free_list = NULL;
      d->arenas = NULL;
    }
}

static struct arena *
slot_to_arena (struct slot *s)
{
  return (struct arena *) ((uint32_t) s & ~(PGSIZE - 1));
}

static void *
get_free_slot (struct desc *d)
{
  void *block = d->free_list;
  ASSERT (block != NULL);
  d->free_list = d->free_list->next;
  return block;
}

void *
malloc (size_t size) 
{
  struct desc *d;
  struct arena *a;
  size_t ofs;

  if (size == 0)
    return NULL;

  for (d = descs; d < descs + desc_cnt; d++)
    if (size < d->slot_size)
      break;
  if (d == descs + desc_cnt) 
    {
      printk ("can't malloc %zu byte object\n", size);
      return NULL; 
    }
  
  if (d->free_list != NULL)
    return get_free_slot (d);

  a = palloc_get (0);
  if (a == NULL)
    return NULL;

  a->desc = d;
  a->prev = NULL;
  a->next = d->arenas;
  if (d->arenas != NULL)
    d->arenas->prev = a;
  for (ofs = sizeof *a; ofs + d->slot_size <= PGSIZE; ofs += d->slot_size) 
    {
      struct slot *s = (struct slot *) ((uint8_t *) a + ofs);
      s->next = d->free_list;
      d->free_list = s;
    }

  return get_free_slot (d);
}

void
free (void *p) 
{
  struct slot *s = p;
  struct arena *a = slot_to_arena (s);
  struct desc *d = a->desc;

  s->next = d->free_list;
  d->free_list = s;
}
