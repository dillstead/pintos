#ifndef HEADER_LIST_H
#define HEADER_LIST_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct list_elem 
  {
    struct list_elem *prev, *next;
  };

struct list 
  {
    struct list_elem head, tail;
  };

#define list_entry(LIST_ELEM, STRUCT, MEMBER)                              \
        ((STRUCT *) ((uint8_t *) (LIST_ELEM) - offsetof (STRUCT, MEMBER)))

void list_init (struct list *);

struct list_elem *list_begin (struct list *);
struct list_elem *list_end (struct list *);
struct list_elem *list_next (struct list_elem *);
struct list_elem *list_prev (struct list_elem *);

void list_insert (struct list_elem *, struct list_elem *);
void list_splice (struct list_elem *before,
                  struct list_elem *first, struct list_elem *last);
void list_push_front (struct list *, struct list_elem *);
void list_push_back (struct list *, struct list_elem *);

void list_remove (struct list_elem *);
struct list_elem *list_pop_front (struct list *);
struct list_elem *list_pop_back (struct list *);

struct list_elem *list_front (struct list *);
struct list_elem *list_back (struct list *);

size_t list_size (struct list *);
bool list_empty (struct list *);

void list_reverse (struct list *);

typedef bool list_less_func (const struct list_elem *a,
                             const struct list_elem *b, void *aux);

void list_merge (struct list *, struct list *,
                 list_less_func *, void *aux);
void list_sort (struct list *,
                list_less_func *, void *aux);
void list_insert_ordered (struct list *, struct list_elem *,
                          list_less_func *, void *aux);
void list_unique (struct list *,
                  list_less_func *, void *aux);

#endif /* list.h */
