#ifndef VM_FRAMETABLE_H
#define VM_FRAMETABLE_H

#include <stdbool.h>

void frametable_init(void);
bool frametable_load_frame(uint32_t *pd, const void *upage, bool write);
void frametable_unload_frame (uint32_t *pd, const void *upage);
bool frametable_lock_frame(uint32_t *pd, const void *upage, bool write);
void frametable_unlock_frame(uint32_t *pd, const void *upage);

#endif /* vm/frametable.h */
