#include "filesys.h"
#include "bitmap.h"
#include "debug.h"
#include "directory.h"
#include "disk.h"
#include "file.h"
#include "filehdr.h"
#include "lib.h"

#define FREE_MAP_SECTOR 0
#define ROOT_DIR_SECTOR 1

#define NUM_DIR_ENTRIES 10
#define ROOT_DIR_FILE_SIZE (sizeof (struct dir_entry) * NUM_DIR_ENTRIES)

struct disk *filesys_disk;

static struct file free_map_file, root_dir_file;

static void
do_format (void)
{
  struct bitmap free_map;
  struct filehdr *map_hdr, *dir_hdr;
  struct dir dir;

  printk ("Formatting filesystem...");

  /* Create the initial bitmap and reserve sectors for the
     free map and root directory file headers. */
  if (!bitmap_init (&free_map, disk_size (filesys_disk)))
    PANIC ("bitmap creation failed--disk is too large");
  bitmap_mark (&free_map, FREE_MAP_SECTOR);
  bitmap_mark (&free_map, ROOT_DIR_SECTOR);

  /* Allocate data sector(s) for the free map file
     and write its file header to disk. */
  map_hdr = filehdr_allocate (&free_map, bitmap_file_size (&free_map));
  if (map_hdr == NULL)
    PANIC ("free map creation failed--disk is too large");
  filehdr_write (map_hdr, FREE_MAP_SECTOR);
  filehdr_destroy (map_hdr);

  /* Allocate data sector(s) for the root directory file
     and write its file header to disk. */
  dir_hdr = filehdr_allocate (&free_map, ROOT_DIR_FILE_SIZE);
  if (dir_hdr == NULL)
    PANIC ("root directory creation failed");
  filehdr_write (dir_hdr, ROOT_DIR_SECTOR);
  filehdr_destroy (dir_hdr);

  /* Write out the free map now that we have space reserved
     for it. */
  if (!file_open (&free_map_file, FREE_MAP_SECTOR))
    PANIC ("can't open free map file");
  bitmap_write (&free_map, &free_map_file);
  bitmap_destroy (&free_map);
  file_close (&free_map_file);

  /* Write out the root directory in the same way. */
  if (!file_open (&root_dir_file, ROOT_DIR_SECTOR))
    PANIC ("can't open root directory");
  if (!dir_init (&dir, NUM_DIR_ENTRIES))
    PANIC ("can't initialize root directory");
  dir_write (&dir, &root_dir_file);
  dir_destroy (&dir);
  file_close (&free_map_file);

  printk ("done.\n");
}

void
filesys_init (bool format) 
{
  filesys_disk = disk_get (0, 1);
  if (filesys_disk == NULL)
    PANIC ("hd0:1 (hdb) not present, filesystem initialization failed");

  if (format) 
    do_format ();
  
  if (!file_open (&free_map_file, FREE_MAP_SECTOR))
    PANIC ("can't open free map file");
  if (!file_open (&root_dir_file, ROOT_DIR_SECTOR))
    PANIC ("can't open root dir file");
}

bool
filesys_create (const char *name, off_t initial_size) 
{
  struct dir dir;
  struct bitmap free_map;
  disk_sector_t hdr_sector;
  struct filehdr *filehdr;
  bool success = false;

  /* Read the root directory. */
  if (!dir_init (&dir, NUM_DIR_ENTRIES))
    return false;
  dir_read (&dir, &root_dir_file);
  if (dir_lookup (&dir, name, NULL)) 
    goto exit1;

  /* Allocate a block for the file header. */
  if (!bitmap_init (&free_map, disk_size (filesys_disk)))
    goto exit1;
  bitmap_read (&free_map, &free_map_file);
  hdr_sector = bitmap_find_and_set (&free_map);
  if (hdr_sector == BITMAP_ERROR)
    goto exit2;

  /* Add the file to the directory. */
  if (!dir_add (&dir, name, hdr_sector))
    goto exit2;

  /* Allocate space for the file. */
  filehdr = filehdr_allocate (&free_map, initial_size);
  if (filehdr == NULL)
    goto exit2;

  /* Write everything back. */
  filehdr_write (filehdr, hdr_sector);
  dir_write (&dir, &root_dir_file);
  bitmap_write (&free_map, &free_map_file);

  success = true;

  /* Clean up. */
  filehdr_destroy (filehdr);
 exit2:
  bitmap_destroy (&free_map);
 exit1:
  dir_destroy (&dir);

  return success;
}

bool
filesys_open (const char *name, struct file *file)
{
  struct dir dir;
  disk_sector_t hdr_sector;
  bool success = false;

  if (!dir_init (&dir, NUM_DIR_ENTRIES))
    return false;
  dir_read (&dir, &root_dir_file);
  if (dir_lookup (&dir, name, &hdr_sector))
    success = file_open (file, hdr_sector);
  
  dir_destroy (&dir);
  return success;
}

bool
filesys_remove (const char *name) 
{
  struct dir dir;
  disk_sector_t hdr_sector;
  struct filehdr *filehdr;
  struct bitmap free_map;
  bool success = false;

  /* Read the root directory. */
  if (!dir_init (&dir, NUM_DIR_ENTRIES))
    return false;
  dir_read (&dir, &root_dir_file);
  if (!dir_lookup (&dir, name, &hdr_sector))
    goto exit1;

  /* Read the file header. */
  filehdr = filehdr_read (hdr_sector);
  if (filehdr == NULL)
    goto exit1;

  /* Allocate a block for the file header. */
  if (!bitmap_init (&free_map, disk_size (filesys_disk)))
    goto exit2;
  bitmap_read (&free_map, &free_map_file);

  /* Deallocate. */
  filehdr_deallocate (filehdr, &free_map);
  bitmap_reset (&free_map, hdr_sector);
  dir_remove (&dir, name);

  /* Write everything back. */
  bitmap_write (&free_map, &free_map_file);
  dir_write (&dir, &root_dir_file);

  success = true;

  /* Clean up. */
  bitmap_destroy (&free_map);
 exit2:
  filehdr_destroy (filehdr);
 exit1:
  dir_destroy (&dir);

  return success;
}

bool
filesys_list (void) 
{
  struct dir dir;

  if (!dir_init (&dir, NUM_DIR_ENTRIES))
    return false;
  dir_read (&dir, &root_dir_file);
  dir_list (&dir);
  dir_destroy (&dir);

  return true;
}

bool
filesys_dump (void) 
{
  struct bitmap free_map;
  struct dir dir;  

  printk ("Free map:\n");
  if (!bitmap_init (&free_map, disk_size (filesys_disk)))
    return false;
  bitmap_read (&free_map, &free_map_file);
  bitmap_dump (&free_map);
  bitmap_destroy (&free_map);
  printk ("\n");
  
  if (!dir_init (&dir, NUM_DIR_ENTRIES))
    return false;
  dir_read (&dir, &root_dir_file);
  dir_dump (&dir);
  dir_destroy (&dir);

  return true;
}

static void must_succeed_function (int, int) NO_INLINE;

static void 
must_succeed_function (int line_no, int success) 
{
  if (!success)
    PANIC ("filesys_self_test: operation failed on line %d", line_no);
}

#define MUST_SUCCEED(EXPR) must_succeed_function (__LINE__, EXPR)

void
filesys_self_test (void)
{
  static const char s[] = "This is a test string.";
  struct file file;
  char s2[sizeof s];

  MUST_SUCCEED (filesys_create ("foo", sizeof s));
  MUST_SUCCEED (filesys_open ("foo", &file));
  MUST_SUCCEED (file_write (&file, s, sizeof s) == sizeof s);
  MUST_SUCCEED (file_tell (&file) == sizeof s);
  MUST_SUCCEED (file_length (&file) == sizeof s);
  file_close (&file);

  MUST_SUCCEED (filesys_open ("foo", &file));
  MUST_SUCCEED (file_read (&file, s2, sizeof s2) == sizeof s2);
  MUST_SUCCEED (memcmp (s, s2, sizeof s) == 0);
  MUST_SUCCEED (file_tell (&file) == sizeof s2);
  MUST_SUCCEED (file_length (&file) == sizeof s2);
  file_close (&file);

  MUST_SUCCEED (filesys_remove ("foo"));

  printk ("filesys: self test ok\n");
}
