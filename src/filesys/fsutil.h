#ifndef FILESYS_FSUTIL_H
#define FILESYS_FSUTIL_H

#include <stdbool.h>

extern char *fsutil_copy_arg;
extern char *fsutil_print_file;
extern char *fsutil_remove_file;
extern bool fsutil_list_files;
extern bool fsutil_dump_filesys;

void fsutil_run (void);
void fsutil_print (const char *filename);

#endif /* filesys/fsutil.h */
