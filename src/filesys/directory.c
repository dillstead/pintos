#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/synch.h"

/* A single directory entry. */
struct dir_entry 
{
  block_sector_t inode_sector;        /* Sector number of header. */
  char name[NAME_MAX + 1];            /* Null terminated file name. */
  bool in_use;                        /* In use or free? */
};

/* Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR.  Returns true if successful, false on failure. */
bool
dir_create (block_sector_t sector, size_t entry_cnt)
{
  return inode_create (sector, entry_cnt * sizeof (struct dir_entry), true);
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (struct inode *dinode, const char *name,
        struct dir_entry *ep, off_t *ofsp) 
{
  struct dir_entry e;
  size_t ofs;
  
  ASSERT (dinode != NULL);
  ASSERT (name != NULL);

  for (ofs = 0; inode_read_at (dinode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e)
    {
      if (e.in_use && !strcmp (name, e.name)) 
        {
          if (ep != NULL)
            *ep = e;
          if (ofsp != NULL)
            *ofsp = ofs;
          return true;
        }
    }
  return false;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup (struct inode *dinode, const char *name,
            struct inode **inode) 
{
  struct dir_entry e;

  ASSERT (dinode != NULL);
  ASSERT (name != NULL);
  ASSERT (inode != NULL);

  if (lookup (dinode, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
    *inode = NULL;
  return *inode != NULL;
}

/* Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add (struct inode *dinode, const char *name, block_sector_t inode_sector)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dinode != NULL);
  ASSERT (name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dinode, name, NULL, NULL))
    {
      goto done;
    }

  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.
     
     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dinode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  strlcpy (e.name, name, sizeof e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dinode, &e, sizeof e, ofs) == sizeof e;

 done:
  return success;
}

/* Removes any entry for NAME in DIR.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct inode *dinode, const char *name) 
{
  struct dir_entry e;
  off_t ofs;

  ASSERT (dinode != NULL);
  ASSERT (name != NULL);

  /* Find directory entry. */
  if (!lookup (dinode, name, &e, &ofs))
    return false;

  /* Erase directory entry. */
  e.in_use = false;
  if (inode_write_at (dinode, &e, sizeof e, ofs) != sizeof e)
    return false;
  return true;
}

/* Reads the next directory entry in DIR from the offset and stores 
   the name in NAME.  Updates the offset and returns true if successful, 
   false if the directory contains no more entries. */
bool
dir_readdir (struct file *file, char name[NAME_MAX + 1])
{
  struct inode *dinode;
  struct dir_entry e;
  bool success = false;

  ASSERT (file != NULL);
  ASSERT (name != NULL);

  dinode = file_get_inode (file);

  /* Skip '.' and '..'. */
  if (file_tell (file) == 0)
    file_seek (file, 2 * sizeof e);
  inode_lock (dinode);
  while (file_read (file, &e, sizeof e) == sizeof e) 
    {
      if (e.in_use)
        {
          strlcpy (name, e.name, NAME_MAX + 1);
          success = true;
          break;
        }
    }
  inode_unlock (dinode);
  return success;
}

/* Returns true if the directory is empty (except for "." and ".."), false if
   not. */
bool
dir_is_empty (struct inode *dinode)
{
  struct dir_entry e;
  size_t ofs;

  ASSERT (dinode != NULL);

  /* Skip '.' and '..'. */
  for (ofs = 2 * sizeof e;
       inode_read_at (dinode, &e, sizeof e, ofs) == sizeof e; ofs += sizeof e) 
    if (e.in_use)
      return false;
  return true;
}
