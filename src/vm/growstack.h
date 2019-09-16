#ifndef VM_GROWSTACK_H
#define VM_GROWSTACK_H

#include <stdint.h>
#include <stdbool.h>

bool is_stack_access (const void *vaddr);
void maybe_grow_stack (uint32_t *pd, const void *vaddr);

#endif /* vm/growstack.h */
