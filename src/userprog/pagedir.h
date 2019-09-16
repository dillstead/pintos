#ifndef USERPROG_PAGEDIR_H
#define USERPROG_PAGEDIR_H

#include <stdbool.h>
#include <stdint.h>

struct page_info;

uint32_t *pagedir_create (void);
void pagedir_destroy (uint32_t *pd);
bool pagedir_set_page (uint32_t *pd, const void *upage, void *kpage, bool rw);
void *pagedir_get_page (uint32_t *pd, const void *uaddr);
void pagedir_clear_page (uint32_t *pd, const void *upage);
bool pagedir_is_dirty (uint32_t *pd, const void *vpage);
void pagedir_set_dirty (uint32_t *pd, const void *vpage, bool dirty);
bool pagedir_is_accessed (uint32_t *pd, const void *vpage);
void pagedir_set_accessed (uint32_t *pd, const void *vpage, bool accessed);
void pagedir_activate (uint32_t *pd);
void pagedir_unload_page (uint32_t *pd, const void *upage);
bool pagedir_set_info (uint32_t *pd, const void *upage, struct page_info *info);
struct page_info *pagedir_get_info (uint32_t *pd, const void *upage);

#endif /* userprog/pagedir.h */
