#ifndef USERPROG_PAGEDIR_H
#define USERPROG_PAGEDIR_H 1

#include <stdbool.h>
#include <stdint.h>

uint32_t *pagedir_create (void);
void pagedir_destroy (uint32_t *);
bool pagedir_set_page (uint32_t *pagedir, void *upage, void *kpage,
                       bool writable);
void *pagedir_get_page (uint32_t *pagedir, const void *upage);
void pagedir_clear_page (uint32_t *pagedir, void *upage);

void pagedir_activate (uint32_t *pagedir);

#endif /* userprog/pagedir.h */
