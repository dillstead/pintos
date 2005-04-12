#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/fsutil.h"
#include "threads/malloc.h"

/* A directory. */
struct dir 
  {
    size_t entry_cnt;           /* Number of entries. */
    struct dir_entry *entries;  /* Array of entries. */
  };

/* A single directory entry. */
struct dir_entry 
  {
    bool in_use;                        /* In use or free? */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    disk_sector_t inode_sector;         /* Sector number of header. */
  };

/* Returns a new directory that holds ENTRY_CNT entries, if
   successful, or a null pointer if memory is unavailable. */
struct dir *
dir_create (size_t entry_cnt) 
{
  struct dir *d = malloc (sizeof *d);
  if (d != NULL)
    {
      d->entry_cnt = entry_cnt;
      d->entries = calloc (1, sizeof *d->entries * entry_cnt);
      if (d->entries != NULL)
        return d;
      free (d); 
    }
  return NULL;
}

/* Returns the size, in bytes, of a directory with ENTRY_CNT
   entries. */
size_t
dir_size (size_t entry_cnt) 
{
  return entry_cnt * sizeof (struct dir_entry);
}

/* Destroys D and frees associated resources. */
void
dir_destroy (struct dir *d) 
{
  if (d != NULL) 
    {
      free (d->entries);
      free (d); 
    }
}

/* Reads D from FILE.
   D must have already been initialized, to the correct number of
   entries, with dir_init(). */
void
dir_read (struct dir *d, struct file *file) 
{
  ASSERT (d != NULL);
  ASSERT (file != NULL);
  ASSERT (file_length (file) >= (off_t) dir_size (d->entry_cnt));

  file_read_at (file, d->entries, dir_size (d->entry_cnt), 0);
}

/* Writes D to FILE.
   D must have already been initialized, to the correct number of
   entries, with dir_init(). */
void
dir_write (struct dir *d, struct file *file) 
{
  file_write_at (file, d->entries, dir_size (d->entry_cnt), 0);
}

/* Searches D for a file named NAME.
   If successful, returns the file's entry;
   otherwise, returns a null pointer. */
static struct dir_entry *
lookup (const struct dir *d, const char *name) 
{
  size_t i;
  
  ASSERT (d != NULL);
  ASSERT (name != NULL);

  if (strlen (name) > NAME_MAX)
    return NULL;

  for (i = 0; i < d->entry_cnt; i++) 
    {
      struct dir_entry *e = &d->entries[i];
      if (e->in_use && !strcmp (name, e->name))
        return e;
    }
  return NULL;
}

/* Searches D for a file named NAME
   and returns true if one exists, false otherwise.
   If INODE_SECTOR is nonnull, then on success *INODE_SECTOR
   is set to the sector that contains the file's inode. */
bool
dir_lookup (const struct dir *d, const char *name,
            disk_sector_t *inode_sector) 
{
  const struct dir_entry *e;

  ASSERT (d != NULL);
  ASSERT (name != NULL);
  
  e = lookup (d, name);
  if (e != NULL) 
    {
      if (inode_sector != NULL)
        *inode_sector = e->inode_sector;
      return true;
    }
  else
    return false;
}

/* Adds a file named NAME to D, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or if D has no free
   directory entries. */
bool
dir_add (struct dir *d, const char *name, disk_sector_t inode_sector) 
{
  size_t i;
  
  ASSERT (d != NULL);
  ASSERT (name != NULL);
  ASSERT (lookup (d, name) == NULL);

  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  for (i = 0; i < d->entry_cnt; i++)
    {
      struct dir_entry *e = &d->entries[i];
      if (!e->in_use) 
        {
          e->in_use = true;
          strlcpy (e->name, name, sizeof e->name);
          e->inode_sector = inode_sector;
          return true;
        }
    }
  return false;
}

/* Removes any entry for NAME in D.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct dir *d, const char *name) 
{
  struct dir_entry *e;

  ASSERT (d != NULL);
  ASSERT (name != NULL);

  e = lookup (d, name);
  if (e != NULL) 
    {
      e->in_use = false;
      return true;
    }
  else
    return false;
}

/* Prints the names of the files in D to the system console. */
void
dir_list (const struct dir *d)
{
  struct dir_entry *e;
  
  for (e = d->entries; e < d->entries + d->entry_cnt; e++)
    if (e->in_use)
      printf ("%s\n", e->name);
}
