#ifndef HEADER_FILESYS_H
#define HEADER_FILESYS_H 1

#include <stdbool.h>

#ifndef OFF_T
#define OFF_T
typedef int32_t off_t;
#endif

struct file;
void filesys_init (bool format);
bool filesys_create (const char *name, off_t initial_size);
bool filesys_remove (const char *name);
void filesys_list (void);

void filesys_self_test (void);

#endif /* filesys.h */
