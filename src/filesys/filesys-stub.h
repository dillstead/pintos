#ifndef HEADER_FILESYS_STUB_H
#define HEADER_FILESYS_STUB_H 1

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct file;

void filesys_stub_init (void);
void filesys_stub_lock (void);
void filesys_stub_unlock (void);

void filesys_stub_put_bool (bool);
void filesys_stub_put_bytes (const void *, size_t);
void filesys_stub_put_file (struct file *);
void filesys_stub_put_int32 (int32_t);
void filesys_stub_put_string (const char *);
void filesys_stub_put_uint32 (uint32_t);

bool filesys_stub_get_bool (void);
void filesys_stub_get_bytes (void *, size_t);
struct file *filesys_stub_get_file (void);
int32_t filesys_stub_get_int32 (void);
void filesys_stub_match_string (const char *);
uint32_t filesys_stub_get_uint32 (void);

#endif /* filesys-stub.h */
