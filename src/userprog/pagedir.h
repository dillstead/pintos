#ifndef USERPROG_PAGEDIR_H
#define USERPROG_PAGEDIR_H

#include <stdbool.h>
#include <stdint.h>

struct page_info;

void pagedir_print_info (struct page_info *info);
uint32_t *pagedir_create (void);
void pagedir_destroy (uint32_t *pd);
void pagedir_unload_page (uint32_t *pd, const void *upage);
bool pagedir_set_page (uint32_t *pd, const void *upage, void *kpage, bool rw);
bool pagedir_set_info (uint32_t *pd, const void *upage, struct page_info *info);
void *pagedir_get_page (uint32_t *pd, const void *uaddr);
struct page_info *pagedir_get_info (uint32_t *pd, const void *upage);
void pagedir_clear_page (uint32_t *pd, const void *upage);
bool pagedir_is_dirty (uint32_t *pd, const void *vpage);
void pagedir_set_dirty (uint32_t *pd, const void *vpage, bool dirty);
bool pagedir_is_accessed (uint32_t *pd, const void *vpage);
void pagedir_set_accessed (uint32_t *pd, const void *vpage, bool accessed);
void pagedir_activate (uint32_t *pd);

#endif /* userprog/pagedir.h */
