#ifndef USERPROG_PAGEDIR_H
#define USERPROG_PAGEDIR_H 1

#include <stdbool.h>
#include <stdint.h>

uint32_t *pagedir_create (void);
void pagedir_destroy (uint32_t *pd);
bool pagedir_set_page (uint32_t *pd, void *upage, void *kpage, bool rw);
void *pagedir_get_page (uint32_t *pd, const void *upage);
void pagedir_activate (uint32_t *pd);

#endif /* userprog/pagedir.h */
