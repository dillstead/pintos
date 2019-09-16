#ifndef VM_PAGEINFO_H
#define VM_PAGEINFO_H

#include <stdint.h>
#include "filesys/off_t.h"
#include "devices/block.h"

/* Page types. */
/* Page content is zero. */
#define PAGE_TYPE_ZERO    0x01
/* Page content is loaded from a page size chunk of kernel memory. */
#define PAGE_TYPE_KERNEL  0x02
/* Page is backed by a file. */
#define PAGE_TYPE_FILE    0x04

/* If a page is writable, it will be written back to a file or to swap. */
#define WRITABLE_TO_FILE  0x01
#define WRITABLE_TO_SWAP  0x02

struct file;

struct page_info *pageinfo_create (void);
void pageinfo_destroy (struct page_info *page_info);
void pageinfo_set_upage (struct page_info *page_info, const void *upage);
void pageinfo_set_type (struct page_info *page_info, int type);
void pageinfo_set_writable (struct page_info *page_info, int writable);
void pageinfo_set_pagedir (struct page_info *page_info, uint32_t *pd);
void pageinfo_set_fileinfo (struct page_info *page_info, struct file *file, off_t offset_cnt);
void pageinfo_set_kpage (struct page_info *page_info, const void *kpage);

#endif /* vm/pageinfo.h */
