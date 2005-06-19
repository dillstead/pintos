#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/disk.h"

/* The disk that contains the filesystem. */
struct disk *filesys_disk;

static void do_format (void);

/* Initializes the filesystem module.
   If FORMAT is true, reformats the filesystem. */
void
filesys_init (bool format) 
{
  filesys_disk = disk_get (0, 1);
  if (filesys_disk == NULL)
    PANIC ("hd0:1 (hdb) not present, filesystem initialization failed");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the filesystem module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  struct dir *dir;
  disk_sector_t inode_sector = 0;
  bool success = (dir_open_root (&dir)
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir;
  struct inode *inode = NULL;

  if (dir_open_root (&dir))
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = NULL;
  bool success = (dir_open_root (&dir)
                  && dir_remove (dir, name));
  dir_close (dir); 

  return success;
}

/* Prints a list of files in the filesystem to the system
   console.
   Returns true if successful, false on failure,
   which occurs only if an internal memory allocation fails. */
bool
filesys_list (void) 
{
  struct dir *dir = NULL;
  bool success = dir_open_root (&dir);
  if (success)
    dir_list (dir);
  dir_close (dir);

  return success;
}

static void must_succeed_function (int, bool) NO_INLINE;
#define MUST_SUCCEED(EXPR) must_succeed_function (__LINE__, EXPR)

/* Performs basic sanity checks on the filesystem.
   The filesystem should not contain a file named `foo' when
   called. */
void
filesys_self_test (void)
{
  static const char s[] = "This is a test string.";
  static const char zeros[sizeof s] = {0};
  struct file *file;
  char s2[sizeof s];
  int i;

  filesys_remove ("foo");
  for (i = 0; i < 2; i++) 
    {
      /* Create file and check that it contains zeros
         throughout the created length. */
      MUST_SUCCEED (filesys_create ("foo", sizeof s));
      MUST_SUCCEED ((file = filesys_open ("foo")) != NULL);
      MUST_SUCCEED (file_read (file, s2, sizeof s2) == sizeof s2);
      MUST_SUCCEED (memcmp (s2, zeros, sizeof s) == 0);
      MUST_SUCCEED (file_tell (file) == sizeof s);
      MUST_SUCCEED (file_length (file) == sizeof s);
      file_close (file);

      /* Reopen file and write to it. */
      MUST_SUCCEED ((file = filesys_open ("foo")) != NULL);
      MUST_SUCCEED (file_write (file, s, sizeof s) == sizeof s);
      MUST_SUCCEED (file_tell (file) == sizeof s);
      MUST_SUCCEED (file_length (file) == sizeof s);
      file_close (file);

      /* Reopen file and verify that it reads back correctly.
         Delete file while open to check proper semantics. */
      MUST_SUCCEED ((file = filesys_open ("foo")) != NULL);
      MUST_SUCCEED (filesys_remove ("foo"));
      MUST_SUCCEED (filesys_open ("foo") == NULL);
      MUST_SUCCEED (file_read (file, s2, sizeof s) == sizeof s);
      MUST_SUCCEED (memcmp (s, s2, sizeof s) == 0);
      MUST_SUCCEED (file_tell (file) == sizeof s);
      MUST_SUCCEED (file_length (file) == sizeof s);
      file_close (file);
    }
  
  printf ("filesys: self test ok\n");
}

/* If SUCCESS is false, panics with an error complaining about
   LINE_NO. */
static void 
must_succeed_function (int line_no, bool success) 
{
  if (!success)
    PANIC ("filesys_self_test: operation failed on line %d", line_no);
}

/* Formats the filesystem. */
static void
do_format (void)
{
  printf ("Formatting filesystem...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
