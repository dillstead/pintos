#ifndef HEADER_LIST_H
#define HEADER_LIST_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct list_elem 
  {
    struct list_elem *prev, *next;
  }
list_elem;

struct list 
  {
    list_elem head, tail;
  };

#define list_entry(LIST_ELEM, STRUCT, MEMBER)                              \
        ((STRUCT *) ((uint8_t *) (LIST_ELEM) - offsetof (STRUCT, MEMBER)))

void list_init (struct list *);

list_elem *list_begin (struct list *);
list_elem *list_end (struct list *);
list_elem *list_next (list_elem *);
list_elem *list_prev (list_elem *);

void list_insert (list_elem *, list_elem *);
void list_splice (list_elem *before,
                  list_elem *first, list_elem *last);
void list_push_front (struct list *, list_elem *);
void list_push_back (struct list *, list_elem *);

void list_remove (list_elem *);
list_elem *list_pop_front (struct list *);
list_elem *list_pop_back (struct list *);

list_elem *list_front (struct list *);
list_elem *list_back (struct list *);

size_t list_size (struct list *);
bool list_empty (struct list *);

void list_reverse (struct list *);

typedef bool list_less_func (const list_elem *a, const list_elem *b,
                             void *aux);

void list_merge (struct list *, struct list *,
                 list_less_func *, void *aux);
void list_sort (struct list *,
                list_less_func *, void *aux);
void list_insert_ordered (struct list *, list_elem *,
                          list_less_func *, void *aux);
void list_unique (struct list *,
                  list_less_func *, void *aux);

#endif /* list.h */
