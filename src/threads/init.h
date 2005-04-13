#ifndef THREADS_INIT_H
#define THREADS_INIT_H

#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Physical memory size, in 4 kB pages. */
extern size_t ram_pages;

/* Page directory with kernel mappings only. */
extern uint32_t *base_page_dir;

/* -o mlfqs:
   If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler. */
extern bool enable_mlfqs;

#ifdef VM
/* -o random-paging:
   If false (default), use LRU page replacement policy.
   If true, use random page replacement policy. */
extern bool enable_random_paging;
#endif

/* -q: Power off when kernel tasks complete? */
extern bool power_off_when_done;

void power_off (void) NO_RETURN;

#endif /* threads/init.h */
