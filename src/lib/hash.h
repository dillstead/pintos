#ifndef HEADER_HASH_H
#define HEADER_HASH_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "list.h"

typedef list_elem hash_elem;

#define hash_entry(HASH_ELEM, STRUCT, MEMBER)                              \
        ((STRUCT *) ((uint8_t *) (HASH_ELEM) - offsetof (STRUCT, MEMBER)))

typedef bool hash_less_func (const hash_elem *a, const hash_elem *b,
                             void *aux);
typedef unsigned hash_hash_func (const hash_elem *, void *aux);

struct hash 
  {
    size_t elem_cnt;
    size_t bucket_cnt;
    struct list *buckets;
    hash_less_func *less;
    hash_hash_func *hash;
    void *aux;
  };

struct hash_iterator 
  {
    struct hash *hash;
    struct list **bucket;
    hash_elem *elem;
  };

bool hash_init (struct hash *, hash_less_func *, hash_hash_func *, void *aux);
void hash_clear (struct hash *);
void hash_destroy (struct hash *);

hash_elem *hash_insert (struct hash *, hash_elem *);
hash_elem *hash_replace (struct hash *, hash_elem *);
hash_elem *hash_find (struct hash *, hash_elem *);
hash_elem *hash_delete (struct hash *, hash_elem *);

void hash_first (struct hash_iterator *, struct hash *);
hash_elem *hash_next (struct hash_iterator *);
hash_elem *hash_cur (struct hash_iterator *);

size_t hash_size (struct hash *);
bool hash_empty (struct hash *);

unsigned hash_bytes (const void *, size_t);
unsigned hash_string (const char *);
unsigned hash_int (int);

#endif /* hash.h */
