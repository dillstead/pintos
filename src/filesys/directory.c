#include "directory.h"
#include "file.h"
#include "fsutil.h"
#include "lib.h"
#include "malloc.h"

bool
dir_init (struct dir *d, size_t entry_cnt) 
{
  d->entry_cnt = entry_cnt;
  d->entries = calloc (1, sizeof *d->entries * entry_cnt);
  return d->entries != NULL;
}

void
dir_destroy (struct dir *d) 
{
  free (d->entries);
}

static off_t
dir_size (struct dir *d) 
{
  return d->entry_cnt * sizeof *d->entries;
}

void
dir_read (struct dir *d, struct file *file) 
{
  file_read_at (file, d->entries, dir_size (d), 0);
}

void
dir_write (struct dir *d, struct file *file) 
{
  file_write_at (file, d->entries, dir_size (d), 0);
}

static struct dir_entry *
lookup (const struct dir *d, const char *name) 
{
  size_t i;
  
  ASSERT (d != NULL);
  ASSERT (name != NULL);

  if (strlen (name) > FILENAME_LEN_MAX)
    return NULL;

  for (i = 0; i < d->entry_cnt; i++) 
    {
      struct dir_entry *e = &d->entries[i];
      if (e->in_use && !strcmp (name, e->name))
        return e;
    }
  return NULL;
}

bool
dir_lookup (const struct dir *d, const char *name,
            disk_sector_no *filehdr_sector) 
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

bool
dir_add (struct dir *d, const char *name, disk_sector_no filehdr_sector) 
{
  size_t i;
  
  ASSERT (d != NULL);
  ASSERT (name != NULL);
  ASSERT (lookup (d, name) == NULL);

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

void
dir_list (const struct dir *d)
{
  struct dir_entry *e;
  
  for (e = d->entries; e < d->entries + d->entry_cnt; e++)
    if (e->in_use)
      printk ("%s\n", e->name);
}

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
