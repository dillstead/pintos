#ifndef THREADS_PAGING_H
#define THREADS_PAGING_H

#include <stdbool.h>
#include <stdint.h>

void paging_init (void);

uint32_t *pagedir_create (void);
void pagedir_destroy (uint32_t *);
bool pagedir_set_page (uint32_t *pagedir, void *upage, void *kpage,
                       bool writable);
void *pagedir_get_page (uint32_t *pagedir, void *upage);
void pagedir_clear_page (uint32_t *pagedir, void *upage);

void *pagedir_first (uint32_t *pagedir, void **upage);
void *pagedir_next (uint32_t *pagedir, void **upage);

void pagedir_activate (uint32_t *pagedir);

#endif /* threads/paging.h */
