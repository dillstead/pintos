#include "list.h"
#include "../debug.h"

/* Our doubly linked lists have two header elements: the "head"
   just before the first element and the "tail" just after the
   last element.  The `prev' link of the front header is null, as
   is the `next' link of the back header.  Their other two links
   point toward each other via the interior elements of the list.

   An empty list looks like this:

                      +------+     +------+
                  <---| head |<--->| tail |--->
                      +------+     +------+

   A list with two elements in it looks like this:

        +------+     +-------+     +-------+     +------+
    <---| head |<--->|   1   |<--->|   2   |<--->| tail |<--->
        +------+     +-------+     +-------+     +------+

   The symmetry of this arrangement eliminates lots of special
   cases in list processing.  For example, take a look at
   list_remove(): it takes only two pointer assignments and no
   conditionals.  That's a lot simpler than the code would be
   without header elements.

   (Because only one of the pointers in each header element is used,
   we could in fact combine them into a single header element
   without sacrificing this simplicity.  But using two separate
   elements allows us to do a little bit of checking on some
   operations, which can be valuable.) */

/* Returns true if ELEM is a head, false otherwise. */
static inline bool
is_head (list_elem *elem)
{
  return elem != NULL && elem->prev == NULL && elem->next != NULL;
}

/* Returns true if ELEM is an interior element,
   false otherwise. */
static inline bool
is_interior (list_elem *elem)
{
  return elem != NULL && elem->prev != NULL && elem->next != NULL;
}

/* Returns true if ELEM is a tail, false otherwise. */
static inline bool
is_tail (list_elem *elem)
{
  return elem != NULL && elem->prev != NULL && elem->next == NULL;
}

/* Initializes LIST as an empty list. */
void
list_init (struct list *list)
{
  ASSERT (list != NULL);
  list->head.prev = NULL;
  list->head.next = &list->tail;
  list->tail.prev = &list->head;
  list->tail.next = NULL;
}

/* Returns the beginning of LIST.  */
list_elem *
list_begin (struct list *list)
{
  ASSERT (list != NULL);
  return list->head.next;
}

/* Returns the element after ELEM in its list.  If ELEM is the
   last element in its list, returns the list tail.  Results are
   undefined if ELEM is itself a list tail. */
list_elem *
list_next (list_elem *elem)
{
  ASSERT (is_head (elem) || is_interior (elem));
  return elem->next;
}

/* Returns LIST's tail.

   list_end() is often used in iterating through a list from
   front to back.  See the big comment at the top of list.h for
   an example. */
list_elem *
list_end (struct list *list)
{
  ASSERT (list != NULL);
  return &list->tail;
}

/* Returns the LIST's reverse beginning, for iterating through
   LIST in reverse order, from back to front. */
list_elem *
list_rbegin (struct list *list) 
{
  ASSERT (list != NULL);
  return list->tail.prev;
}

/* Returns the element before ELEM in its list.  If ELEM is the
   first element in its list, returns the list head.  Results are
   undefined if ELEM is itself a list head. */
list_elem *
list_prev (list_elem *elem)
{
  ASSERT (is_interior (elem) || is_tail (elem));
  return elem->prev;
}

/* Returns LIST's head.

   list_rend() is often used in iterating through a list in
   reverse order, from back to front.  Here's typical usage,
   following the example from the top of list.h:

      for (e = list_rbegin (&foo_list); e != list_rend (&foo_list);
           e = list_prev (e))
        {
          struct foo *f = list_entry (e, struct foo, elem);
          ...do something with f...
        }
*/
list_elem *
list_rend (struct list *list) 
{
  ASSERT (list != NULL);
  return &list->head;
}

/* Return's LIST's head.

   list_head() can be used for an alternate style of iterating
   through a list, e.g.:

      e = list_head (&list);
      while ((e = list_next (e)) != list_end (&list)) 
        {
          ...
        }
*/
list_elem *
list_head (struct list *list) 
{
  ASSERT (list != NULL);
  return &list->head;
}

/* Return's LIST's tail. */
list_elem *
list_tail (struct list *list) 
{
  ASSERT (list != NULL);
  return &list->tail;
}

/* Inserts ELEM just before BEFORE, which may be either an
   interior element or a tail.  The latter case is equivalent to
   list_push_back(). */
void
list_insert (list_elem *before, list_elem *elem)
{
  ASSERT (is_interior (before) || is_tail (before));
  ASSERT (elem != NULL);

  elem->prev = before->prev;
  elem->next = before;
  before->prev->next = elem;
  before->prev = elem;
}

/* Removes elements FIRST though LAST (exclusive) from their
   current list, then inserts them just before BEFORE, which may
   be either an interior element or a tail. */
void
list_splice (list_elem *before,
             list_elem *first, list_elem *last)
{
  ASSERT (is_interior (before) || is_tail (before));
  if (first == last)
    return;
  last = list_prev (last);

  ASSERT (is_interior (first));
  ASSERT (is_interior (last));

  /* Cleanly remove FIRST...LAST from its current list. */
  first->prev->next = last->next;
  last->next->prev = first->prev;

  /* Splice FIRST...LAST into new list. */
  first->prev = before->prev;
  last->next = before;
  before->prev->next = first;
  before->prev = last;
}

/* Inserts ELEM at the beginning of LIST, so that it becomes the
   front in LIST. */
void
list_push_front (struct list *list, list_elem *elem)
{
  list_insert (list_begin (list), elem);
}

/* Inserts ELEM at the end of LIST, so that it becomes the
   back in LIST. */
void
list_push_back (struct list *list, list_elem *elem)
{
  list_insert (list_end (list), elem);
}

/* Removes ELEM from its list and returns the element that
   followed it.  Undefined behavior if ELEM is not in a list.  */
list_elem *
list_remove (list_elem *elem)
{
  ASSERT (is_interior (elem));
  elem->prev->next = elem->next;
  elem->next->prev = elem->prev;
  return elem->next;
}

/* Removes the front element from LIST and returns it.
   Undefined behavior if LIST is empty before removal. */
list_elem *
list_pop_front (struct list *list)
{
  list_elem *front = list_front (list);
  list_remove (front);
  return front;
}

/* Removes the back element from LIST and returns it.
   Undefined behavior if LIST is empty before removal. */
list_elem *
list_pop_back (struct list *list)
{
  list_elem *back = list_back (list);
  list_remove (back);
  return back;
}

/* Returns the front element in LIST.
   Undefined behavior if LIST is empty. */
list_elem *
list_front (struct list *list)
{
  ASSERT (!list_empty (list));
  return list->head.next;
}

/* Returns the back element in LIST.
   Undefined behavior if LIST is empty. */
list_elem *
list_back (struct list *list)
{
  ASSERT (!list_empty (list));
  return list->tail.prev;
}

/* Returns the number of elements in LIST.
   Runs in O(n) in the number of elements. */
size_t
list_size (struct list *list)
{
  list_elem *e;
  size_t cnt = 0;

  for (e = list_begin (list); e != list_end (list); e = list_next (e))
    cnt++;
  return cnt;
}

/* Returns true if LIST is empty, false otherwise. */
bool
list_empty (struct list *list)
{
  return list_begin (list) == list_end (list);
}

/* Swaps the `list_elem *'s that A and B point to. */
static void
swap (list_elem **a, list_elem **b) 
{
  list_elem *t = *a;
  *a = *b;
  *b = t;
}

/* Reverses the order of LIST. */
void
list_reverse (struct list *list)
{
  if (!list_empty (list)) 
    {
      list_elem *e;

      for (e = list_begin (list); e != list_end (list); e = e->prev)
        swap (&e->prev, &e->next);
      swap (&list->head.next, &list->tail.prev);
      swap (&list->head.next->prev, &list->tail.prev->next);
    }
}

/* Merges lists AL and BL, which must each be sorted according to
   LESS given auxiliary data AUX, by inserting each element of BL
   at the proper place in AL to preserve the ordering.
   Runs in O(n) in the combined length of AL and BL. */
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

/* Sorts LIST according to LESS given auxiliary data AUX.
   Runs in O(n lg n) in the number of elements in LIST. */
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
  middle = last = list_begin (list);
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

/* Inserts ELEM in the proper position in LIST, which must be
   sorted according to LESS given auxiliary data AUX.
   Runs in O(n) average case in the number of elements in LIST. */
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

/* Iterates through LIST and removes all but the first in each
   set of adjacent elements that are equal according to LESS
   given auxiliary data AUX.  If DUPLICATES is non-null, then the
   elements from LIST are appended to DUPLICATES. */
void
list_unique (struct list *list, struct list *duplicates,
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
      {
        list_remove (next);
        if (duplicates != NULL)
          list_push_back (duplicates, next);
      }
    else
      elem = next;
}
