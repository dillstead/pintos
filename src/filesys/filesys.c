#include "filesys.h"
#include "disk.h"
#include "directory.h"

static struct disk *disk;

static struct file free_map_file, root_dir_file;

#define FREE_MAP_SECTOR 0
#define ROOT_DIR_SECTOR 1

#define NUM_DIR_ENTRIES 10
#define ROOT_DIR_FILE_SIZE (sizeof (struct dir_entry) * NUM_DIR_ENTRIES)

static void
do_format (void)
{
  struct bitmap free_map;
  struct filehdr map_hdr, dir_hdr;
  struct dir dir;

  /* Create the initial bitmap and reserve sectors for the
     free map and root directory file headers. */
  if (!bitmap_init (&free_map, disk_size (disk)))
    panic ("bitmap creation failed");
  bitmap_mark (&free_map, FREE_MAP_SECTOR);
  bitmap_mark (&free_map, ROOT_DIR_SECTOR);

  /* Allocate data sector(s) for the free map file
     and write its file header to disk. */
  if (!filehdr_allocate (&map_hdr, bitmap_storage_size (&free_map)))
    panic ("free map creation failed");
  filehdr_write (&map_hdr, FREE_MAP_SECTOR);
  filehdr_destroy (&map_hdr);

  /* Allocate data sector(s) for the root directory file
     and write its file header to disk. */
  if (!filehdr_allocate (&dir_hdr, ROOT_DIR_FILE_SIZE))
    panic ("root directory creation failed");
  filehdr_write (&dir_hdr, FREE_MAP_SECTOR);
  filehdr_destroy (&dir_hdr);

  /* Write out the free map now that we have space reserved
     for it. */
  file_open (&free_map_file, FREE_MAP_SECTOR);
  bitmapio_write (&free_map, free_map_file);
  bitmap_destroy (&free_map);
  file_close (&free_map_file);

  /* Write out the root directory in the same way. */
  file_open (&root_dir_file, ROOT_DIR_SECTOR);
  if (!dir_init (&dir, NUM_DIR_ENTRIES))
    panic ("can't initialize root directory");
  dir_write (root_dir_file);
  dir_destroy (&dir);
  file_close (&free_map_file);
}

void
filesys_init (bool format) 
{
  disk = disk_get (1);
  if (disk == NULL)
    panic ("ide1:1 not present, filesystem initialization failed");

  if (format) 
    do_format ();
  
  file_open (&free_map_file, FREE_MAP_SECTOR);
  file_open (&root_dir_file, ROOT_DIR_SECTOR);
}

bool
filesys_create (const char *name, off_t initial_size) 
{
  struct dir dir;
  struct bitmap free_map;
  disk_sector_no hdr_sector;
  struct filehdr filehdr;
  bool success = false;

  /* Read the root directory. */
  dir_init (&dir, NUM_DIR_ENTRIES);
  dir_read (&dir, &root_dir_file);
  if (dir_lookup (&dir, name, NULL)) 
    goto exit1;

  /* Allocate a block for the file header. */
  if (!bitmap_init (&free_map, disk_size (disk)))
    goto exit1;
  bitmapio_read (&free_map, &free_map_file);
  hdr_sector = bitmap_find_and_set (&free_map);
  if (hdr_sector == BITMAP_ERROR)
    goto exit2;

  /* Add the file to the directory. */
  if (!dir_add (&dir, name, hdr_sector))
    goto exit2;

  /* Allocate space for the file. */
  if (!filehdr_allocate (&filehdr, initial_size))
    goto exit2;

  /* Write everything back. */
  filehdr_write (&filehdr, hdr_sector);
  dir_write (&dir, &root_dir_file);
  bitmapio_write (&free_map, &free_map_file);

  success = true;

  /* Clean up. */
  filehdr_destroy (&filehdr);
 exit2:
  bitmap_destroy (&free_map);
 exit1:
  dir_destroy (&dir);

  return success;
}

bool
filesys_remove (const char *name) 
{
  struct dir dir;
  disk_sector_no hdr_sector;
  struct filehdr filehdr;
  struct bitmap free_map;
  bool success = false;

  /* Read the root directory. */
  dir_init (&dir, NUM_DIR_ENTRIES);
  dir_read (&dir, &root_dir_file);
  if (!dir_lookup (&dir, name, &hdr_sector))
    goto exit1;

  /* Read the file header. */
  if (!filehdr_read (&filehdr, hdr_sector))
    goto exit1;

  /* Allocate a block for the file header. */
  if (!bitmap_init (&free_map, disk_size (disk)))
    goto exit2;
  bitmapio_read (&free_map, &free_map_file);

  /* Deallocate. */
  filehdr_deallocate (&filehdr, &free_map);
  bitmap_reset (&free_map, hdr_sector);
  dir_remove (&dir, name);

  /* Write everything back. */
  bitmapio_write (&free_map, &free_map_file);
  dir_write (&dir, &root_dir_file);

  success = true;

  /* Clean up. */
  bitmap_destroy (&free_map);
 exit2:
  filehdr_destroy (&filehdr);
 exit1:
  dir_destroy (&dir);

  return success;
}

#undef NDEBUG
#include "debug.h"
#include "file.h"

void
filesys_self_test (void)
{
  static const char s[] = "This is a test string.";
  struct file *file;
  char s2[sizeof s];

  ASSERT (filesys_create ("foo"));
  ASSERT ((file = filesys_open ("foo")) != NULL);
  ASSERT (file_write (file, s, sizeof s) == sizeof s);
  ASSERT (file_tell (file) == sizeof s);
  ASSERT (file_length (file) == sizeof s);
  file_close (file);

  ASSERT ((file = filesys_open ("foo")) != NULL);
  ASSERT (file_read (file, s2, sizeof s2) == sizeof s2);
  ASSERT (memcmp (s, s2, sizeof s) == 0);
  ASSERT (file_tell (file) == sizeof s2);
  ASSERT (file_length (file) == sizeof s2);
  file_close (file);

  ASSERT (filesys_remove ("foo"));
}
