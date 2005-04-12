#ifndef FILESYS_FSUTIL_H
#define FILESYS_FSUTIL_H

#include <stdbool.h>

extern char *fsutil_copyin_file;
extern int fsutil_copyin_size;
extern char *fsutil_copyout_file;
extern char *fsutil_print_file;
extern char *fsutil_remove_file;
extern bool fsutil_list_files;

void fsutil_run (void);
void fsutil_print (const char *filename);

#endif /* filesys/fsutil.h */
