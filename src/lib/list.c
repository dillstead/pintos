#include "list.h"
#include "debug.h"

void
list_init (struct list *list) 
{
  list->head.prev = NULL;
  list->head.next = &list->tail;
  list->tail.prev = &list->head;
  list->tail.next = NULL;
}

list_elem *
list_begin (struct list *list) 
{
  return list->head.next;
}

list_elem *
list_end (struct list *list) 
{
  return &list->tail;
}

static inline bool
is_head (list_elem *elem) 
{
  return elem != NULL && elem->prev == NULL && elem->next != NULL;
}

static inline bool
is_real (list_elem *elem) 
{
  return elem != NULL && elem->prev != NULL && elem->next != NULL;
}

static inline bool
is_tail (list_elem *elem) 
{
  return elem != NULL && elem->prev != NULL && elem->next == NULL;
}

list_elem *
list_next (list_elem *elem) 
{
  ASSERT (is_real (elem));
  return elem->next;
}

list_elem *
list_prev (list_elem *elem) 
{
  ASSERT (is_real (elem) || is_tail (elem));
  return elem->prev;
}

void
list_insert (list_elem *before, list_elem *elem) 
{
  ASSERT (is_real (before) || is_tail (before));
  ASSERT (elem != NULL);

  elem->prev = before->prev;
  elem->next = before;
  before->prev->next = elem;
  before->prev = elem;
}

void
list_splice (list_elem *target,
             list_elem *first, list_elem *last) 
{
  ASSERT (is_real (target) || is_tail (target));
  if (first == last)
    return;
  last = list_prev (last);

  ASSERT (is_real (first));
  ASSERT (is_real (last));

  /* Cleanly remove FIRST...LAST from its current list. */
  first->prev->next = last->next;
  last->next->prev = first->prev;

  /* Splice FIRST...LAST into new list. */
  first->prev = target->prev;
  last->next = target;
  target->prev->next = first;
  target->prev = last;
}

void
list_push_front (struct list *list, list_elem *elem) 
{
  list_insert (list_begin (list), elem);
}

void
list_push_back (struct list *list, list_elem *elem)
{
  list_insert (list_end (list), elem);
}

static list_elem *
remove_item (list_elem *elem)
{
  ASSERT (is_real (elem));
  elem->prev->next = elem->next;
  elem->next->prev = elem->prev;
  return elem;
}

void
list_remove (list_elem *elem) 
{
  remove_item (elem);
}

list_elem *
list_pop_front (struct list *list) 
{
  return remove_item (list_front (list));
}

list_elem *
list_pop_back (struct list *list) 
{
  return remove_item (list_back (list));
}

list_elem *
list_front (struct list *list) 
{
  return list_begin (list);
}

list_elem *
list_back (struct list *list)
{
  return list_prev (list_end (list));
}

size_t
list_size (struct list *list)
{
  list_elem *elem;
  size_t cnt = 0;
  
  for (elem = list_begin (list); elem != list_end (list); elem = elem->next) 
    cnt++;
  return cnt;
}

bool
list_empty (struct list *list) 
{
  return list_begin (list) == list_end (list);
}

void
list_reverse (struct list *list) 
{
  list_elem te, *e;
  
  for (e = &list->head; e != NULL; e = e->prev)
    {
      list_elem *tep = e->prev;
      e->prev = e->next;
      e->next = tep;
    }

  te = list->head;
  list->head = list->tail;
  list->tail = te;
}

void
list_merge (struct list *al, struct list *bl,
            list_less_func *less, void *aux)
{
  list_elem *a;

  ASSERT (al != NULL);
  ASSERT (bl != NULL);
  ASSERT (less != NULL);

  a = list_begin (al);
  while (a != list_end (al))
    {
      list_elem *b = list_begin (bl); 
      if (less (b, a, aux)) 
        {
          list_splice (a, b, list_next (b));
          if (list_empty (bl))
            break; 
        }
      else
        a = list_next (a);
    }
  list_splice (list_end (al), list_begin (bl), list_end (bl));
}

void
list_sort (struct list *list,
           list_less_func *less, void *aux)
{
  struct list tmp;
  list_elem *middle, *last;

  ASSERT (list != NULL);
  ASSERT (less != NULL);

  /* Empty and 1-element lists are already sorted. */
  if (list_empty (list) || list_next (list_begin (list)) == list_end (list))
    return;

  /* Find middle of LIST.  (We're not interested in the end of
     the list but it's incidentally needed.) */
  middle = list_begin (list);
  last = list_next (middle);
  while (last != list_end (list) && list_next (last) != list_end (list)) 
    {
      middle = list_next (middle);
      last = list_next (list_next (last));
    }

  /* Extract first half of LIST into a temporary list. */
  list_init (&tmp);
  list_splice (list_begin (&tmp), list_begin (list), middle);

  /* Sort each half-list and merge the result. */
  list_sort (&tmp, less, aux);
  list_sort (list, less, aux);
  list_merge (list, &tmp, less, aux);
}

void
list_insert_ordered (struct list *list, list_elem *elem,
                     list_less_func *less, void *aux) 
{
  list_elem *e;

  ASSERT (list != NULL);
  ASSERT (elem != NULL);
  ASSERT (less != NULL);
  
  for (e = list_begin (list); e != list_end (list); e = list_next (e))
    if (less (elem, e, aux))
      break;
  return list_insert (e, elem);
}

void
list_unique (struct list *list,
             list_less_func *less, void *aux)
{
  list_elem *elem, *next;

  ASSERT (list != NULL);
  ASSERT (less != NULL);
  if (list_empty (list))
    return;

  elem = list_begin (list);
  while ((next = list_next (elem)) != list_end (list))
    if (!less (elem, next, aux) && !less (next, elem, aux))
      list_remove (next);
    else
      elem = next;
}
