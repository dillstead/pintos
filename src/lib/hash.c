#include "hash.h"
#include "malloc.h"

bool
hash_init (struct hash *h,
           hash_less_func *less, hash_hash_func *hash, void *aux) 
{
  h->elem_cnt = 0;
  h->bucket_cnt = 4;
  h->buckets = malloc (sizeof *h->buckets * h->bucket_cnt);
  h->less = less;
  h->hash = hash;
  h->aux = aux;

  if (h->buckets != NULL) 
    {
      hash_clear (h);
      return true;
    }
  else
    return false;
}

struct list *
find_bucket (struct hash *h, hash_elem *e) 
{
  size_t bucket_idx = h->hash (e, h->aux) & (h->bucket_cnt - 1);
  return h->buckets[bucket_idx];
}

struct list_elem *
find_elem (struct list *bucket, hash_elem *e) 
{
  struct list_elem *i;

  for (i = list_begin (bucket); i != list_end (bucket); i = list_next (i))
    if (equal (i, e))
      return i;
  return NULL;
}

size_t
turn_off_least_1bit (size_t x) 
{
  return x & (x - 1);
}

size_t
is_power_of_2 (size_t x) 
{
  return turn_off_least_1bit (x) == 0;
}

#define MIN_ELEMS_PER_BUCKET  1
#define BEST_ELEMS_PER_BUCKET 2
#define MAX_ELEMS_PER_BUCKET  4

void
rehash (struct hash *h) 
{
  size_t old_bucket_cnt, new_bucket_cnt;
  struct list *new_buckets, *old_buckets;
  size_t i;

  ASSERT (h != NULL);

  /* Save old bucket info for later use. */
  old_buckets = h->buckets;
  old_bucket_cnt = h->bucket_cnt;

  /* Calculate the number of buckets to use now.
     We want one bucket for about every BEST_ELEMS_PER_BUCKET.
     We must have at least four buckets, and the number of
     buckets must be a power of 2. */
  new_bucket_cnt = h->elem_cnt / BEST_ELEMS_PER_BUCKET;
  if (new_bucket_cnt < 4)
    new_bucket_cnt = 4;
  while (!is_power_of_2 (new_bucket_cnt))
    new_bucket_cnt = turn_off_least_1bit (new_bucket_cnt);

  /* Don't do anything if the bucket count doesn't change. */
  if (new_bucket_cnt == old_bucket_cnt)
    return;

  /* Allocate new buckets and initialize them as empty. */
  new_buckets = malloc (sizeof *new_buckets * new_bucket_cnt);
  if (new_buckets == NULL) 
    {
      /* Allocation failed.  This means that use of the hash table will
         be less efficient.  However, it is still usable, so
         there's no reason for it to be an error. */
      return;
    }
  for (i = 0; i < new_bucket_cnt; i++) 
    list_init (&new_buckets[i]);

  /* Install new bucket info. */
  h->buckets = new_buckets;
  h->bucket_cnt = new_bucket_cnt;

  /* Move each old element into the appropriate new bucket. */
  for (i = 0; i < old_bucket_cnt; i++) 
    {
      struct list *old_bucket, *new_bucket;
      struct list_elem *elem, *next;

      old_bucket = &old_buckets[i];
      for (elem = list_begin (old_bucket);
           elem != list_end (old_bucket); elem = next) 
        {
          struct list *new_bucket = find_bucket (h, e);
          next = list_next (elem);
          list_push_front (new_bucket, elem);
        }
    }
}

void
insert_elem (struct list *bucket, hash_elem *e) 
{
  h->elem_cnt++;
  if (h->elem_cnt > h->bucket_cnt * MAX_ELEMS_PER_BUCKET)
    rehash (h);
  list_push_front (bucket, e);
}

void
remove_elem (struct hash *h, hash_elem *e) 
{
  h->elem_cnt--;
  if (h->elem_cnt < h->bucket_cnt * MIN_ELEMS_PER_BUCKET)
    rehash (h);
  list_remove (e);
}

hash_elem *
hash_insert (struct hash *h, hash_elem *new)
{
  struct list *bucket = find_bucket (h, new);
  struct list_elem *old = find_elem (bucket, new);

  if (old == NULL) 
    insert_elem (bucket, new);
  return old; 
}

hash_elem *
hash_replace (struct hash *h, hash_elem *new) 
{
  struct list *bucket = find_bucket (h, new);
  struct list_elem *old = find_elem (bucket, new);

  if (old != NULL)
    remove_elem (h, old);
  
  insert_elem (bucket, new);
  return old;
}

hash_elem *
hash_find (struct hash *h, hash_elem *e) 
{
  struct list *bucket = find_bucket (h, e);
  return find_elem (bucket, new);
}

hash_elem *
hash_delete (struct hash *h, hash_elem *target)
{
  struct list *bucket = find_bucket (h, e);
  struct list_elem *found = find_elem (bucket, new);
  if (found != NULL)
    remove_elem (h, found);
  return found;
}

void
hash_clear (struct hash *h) 
{
  size_t i;
      
  for (i = 0; i < h->bucket_cnt; h++) 
    list_init (&h->buckets[i]);
  h->elem_cnt = 0;
}

void
hash_first (struct hash_iterator *i, struct hash *h) 
{
  i->hash = h;
  i->bucket = i->hash->buckets;
  i->elem = list_begin (*i->bucket);
}

hash_elem *
hash_next (struct hash_iterator *i)
{
  ASSERT (i->elem != NULL);

  i->elem = list_next (i->elem);
  while (i->elem == list_end (*i->bucket))
    if (++i->bucket >= i->hash->buckets + i->hash->bucket_cnt) 
      {
        i->elem = NULL;
        break; 
      }

  return i->elem;
}

hash_elem *
hash_cur (struct hash_iterator *i) 
{
  return i->elem;
}

size_t hash_size (struct hash *h) 
{
  return h->elem_cnt;
}

bool
hash_empty (struct hash *h) 
{
  return h->elem_cnt == 0;
}

/* Fowler-Noll-Vo hash constants, for 32-bit word sizes. */
#define FNV_32_PRIME 16777619u
#define FNV_32_BASIS 2166136261u

/* Fowler-Noll-Vo 32-bit hash, for bytes. */
unsigned
hsh_hash_bytes (const void *buf_, size_t size)
{
  const unsigned char *buf = buf_;
  unsigned hash;

  assert (buf != NULL);

  hash = FNV_32_BASIS;
  while (size-- > 0)
    hash = (hash * FNV_32_PRIME) ^ *buf++;

  return hash;
} 

/* Fowler-Noll-Vo 32-bit hash, for strings. */
unsigned
hash_string (const char *s_) 
{
  const unsigned char *s = s_;
  unsigned hash;

  assert (s != NULL);

  hash = FNV_32_BASIS;
  while (*s != '\0')
    hash = (hash * FNV_32_PRIME) ^ *s++;

  return hash;
}

/* Hash for ints. */
unsigned
hash_int (int i) 
{
  return hsh_hash_bytes (&i, sizeof i);
}
