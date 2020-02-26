#include <stdio.h>


#include <string.h>
#include "threads/thread.h"
#include "filesys/path.h"
#include "filesys/inode.h"
#include "filesys/filesys.h"
#include "filesys/directory.h"


#include "threads/thread.h"

/* The following routines were taken from:
     https://github.com/mit-pdos/xv6-public/blob/master/fs.c
     and lightly modified. */

/* Copy the next path element from path into name.
   Return a pointer to the element following the copied one.
   The returned path has no leading slashes,
   so the caller can check *path=='\0' to see if the name is the last one.
   If no name to remove, return NULL;

   Examples:
     skip_elem("a/bb/c", name) = "bb/c", setting name = "a"
     skip_elem("///a//bb", name) = "bb", setting name = "a"
     skip_elem("a", name) = "", setting name = "a"
     skip_elem("", name) = skipelem("////", name) = 0 */
static const char *
skip_elem (const char *path, char name[NAME_MAX + 1], bool *len_exceeded)
{
  const char *s;
  int len;

  *len_exceeded = false;
  while (*path == '/')
    path++;
  if (*path == '\0')
    return NULL;
  s = path;
  while (*path != '/' && *path != '\0')
    path++;
  len = path - s;
  if (len > NAME_MAX)
    {
      len = NAME_MAX;
      *len_exceeded = true;
    }
  memcpy(name, s, len);
  name[len] = '\0';
  while (*path == '/')
    path++;
  return path;
}

static void
unlock_close (struct inode *inode)
{
  inode_unlock (inode);
  inode_close (inode);  
}

static struct inode *
lookup (const char *path, bool parent, char name[NAME_MAX + 1])
{
  struct inode *inode;
  struct inode *next;
  bool len_exceeded;
#ifdef DEBUG_PATH
  printf ("lookup enter %s%d, pth: %s, par: %d\n",
          thread_name (), thread_current ()->tid,
          path, parent);
#endif
  if (*path == '/')
    inode = inode_open (ROOT_DIR_SECTOR);
  else
    inode = inode_reopen (thread_current ()->cwd);
  path = skip_elem (path, name, &len_exceeded);
  while (path != NULL && !len_exceeded)
    {
      inode_lock (inode);
      if (!inode_is_dir (inode) || inode_is_removed (inode))
        {
          unlock_close (inode);
#ifdef DEBUG_PATH
          printf ("lookup exit %s%d, inode: NULL\n",
                  thread_name (), thread_current ()->tid);
#endif
          return NULL;
        }
      /* Stop one level early if looking for the parent. */
      if (parent && *path == '\0')
        {
#ifdef DEBUG_PATH
          printf ("lookup exit %s%d, inode: %p\n",
                  thread_name (), thread_current ()->tid,
                  inode);
#endif
          inode_unlock (inode);
          return inode;
        }
      if (!dir_lookup (inode, name, &next))
        {
          unlock_close (inode);
#ifdef DEBUG_PATH
          printf ("lookup exit %s%d, inode: NULL\n",
                  thread_name (), thread_current ()->tid);
#endif                    
          return NULL;
        }
      unlock_close (inode);
      inode = next;
      path = skip_elem (path, name, &len_exceeded);
    }
  if (parent || len_exceeded)
    {
      inode_close (inode);
#ifdef DEBUG_PATH
      printf ("lookup exit %s%d, inode: NULL\n",
              thread_name (), thread_current ()->tid);
#endif          
      return NULL;
    }
#ifdef DEBUG_PATH
  printf ("lookup exit %s%d, inode: %p\n",
          thread_name (), thread_current ()->tid, inode);
#endif          
  return inode;
}

/* Returns true if the path ends in a '/'. */
bool
path_has_trailing_slash (const char *path)
{
  char c = '\0';

  while (*path != '\0')
    {
      c = *path;
      path++;
    }

  return c == '/';
}

bool
path_is_empty (const char *path)
{
  return (path == NULL || *path == '\0');
}

/* Look up and return the inode for a path name. */
struct inode *
path_lookup (const char *path)
{
  char name[NAME_MAX + 1];

  return lookup (path, false, name);
}

/* Look up and return the inode for the parent and copy the final path element
   into name, which must have room for NAME_MAX + 1 bytes. */
struct inode *
path_lookup_parent (const char *path, char name[NAME_MAX + 1])
{
  return lookup (path, true, name);
}
