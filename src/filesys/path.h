#ifndef FILESYS_PATH_H
#define FILESYS_PATH_H

#include "filesys/directory.h"

bool path_has_trailing_slash (const char *path);
bool path_is_empty (const char *path);
struct inode *path_lookup (const char *path);
struct inode *path_lookup_parent (const char *path, char name[NAME_MAX + 1]);

#endif /* filesys/path.h */
