#include "directory.h"
#include "file.h"
#include "fsutil.h"
#include "lib/lib.h"
#include "threads/malloc.h"

/* Initializes D as a directory that holds ENTRY_CNT entries. */
bool
dir_init (struct dir *d, size_t entry_cnt) 
{
  ASSERT (d != NULL);
  d->entry_cnt = entry_cnt;
  d->entries = calloc (1, sizeof *d->entries * entry_cnt);
  return d->entries != NULL;
}

/* Destroys D and frees associated resources. */
void
dir_destroy (struct dir *d) 
{
  if (d != NULL) 
    free (d->entries);
}

/* Returns the size of D in bytes. */
static off_t
dir_size (struct dir *d) 
{
  ASSERT (d != NULL);
  return d->entry_cnt * sizeof *d->entries;
}

/* Reads D from FILE.
   D must have already been initialized, to the correct number of
   entries, with dir_init(). */
void
dir_read (struct dir *d, struct file *file) 
{
  ASSERT (d != NULL);
  ASSERT (file != NULL);
  ASSERT (file_length (file) >= dir_size (d));

  file_read_at (file, d->entries, dir_size (d), 0);
}

/* Writes D to FILE.
   D must have already been initialized, to the correct number of
   entries, with dir_init(). */
void
dir_write (struct dir *d, struct file *file) 
{
  file_write_at (file, d->entries, dir_size (d), 0);
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
   If FILEHDR_SECTOR is nonnull, then on success *FILEHDR_SECTOR
   is set to the sector that contains the file's header. */
bool
dir_lookup (const struct dir *d, const char *name,
            disk_sector_t *filehdr_sector) 
{
  const struct dir_entry *e;

  ASSERT (d != NULL);
  ASSERT (name != NULL);
  
  e = lookup (d, name);
  if (e != NULL) 
    {
      if (filehdr_sector != NULL)
        *filehdr_sector = e->filehdr_sector;
      return true;
    }
  else
    return false;
}

/* Adds a file named NAME to D, which must not already contain a
   file by that name.  The file's header is in sector
   FILEHDR_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or if D has no free
   directory entries. */
bool
dir_add (struct dir *d, const char *name, disk_sector_t filehdr_sector) 
{
  size_t i;
  
  ASSERT (d != NULL);
  ASSERT (name != NULL);
  ASSERT (lookup (d, name) == NULL);

  if (strlen (name) > NAME_MAX)
    return false;

  for (i = 0; i < d->entry_cnt; i++)
    {
      struct dir_entry *e = &d->entries[i];
      if (!e->in_use) 
        {
          e->in_use = true;
          strlcpy (e->name, name, sizeof e->name);
          e->filehdr_sector = filehdr_sector;
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
      printk ("%s\n", e->name);
}

/* Dumps the contents of D, including its files' names and their
   contents, to the system console. */
void
dir_dump (const struct dir *d) 
{
  struct dir_entry *e;
  
  for (e = d->entries; e < d->entries + d->entry_cnt; e++)
    if (e->in_use) 
      {
        printk ("Contents of %s:\n", e->name);
        fsutil_print (e->name);
        printk ("\n");
      }
}
