#ifndef HEADER_FSUTIL_H
#define HEADER_FSUTIL_H 1

#include <stdbool.h>

extern char *fsutil_copy_arg;
extern char *fsutil_print_file;
extern char *fsutil_remove_file;
extern bool fsutil_list_files;
extern bool fsutil_dump_filesys;

void fsutil_run (void);
void fsutil_print (const char *filename);

#endif /* fsutil.h */
