/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "filesys.h"
#include "bitmap.h"
#include "debug.h"
#include "directory.h"
#include "disk.h"
#include "file.h"
#include "filehdr.h"
#include "lib.h"

/* Filesystem.

   The filesystem consists of a set of files.  Each file has a
   header, represented by struct filehdr, that is stored by
   itself in a single sector (see filehdr.h).  The header
   contains the file's length in bytes and an array that lists
   the sector numbers for the file's contents.

   Two files are special.  The first special file is the free
   map, whose header is always stored in sector 0
   (FREE_MAP_SECTOR).  The free map stores a bitmap (see
   lib/bitmap.h) that contains one bit for each sector on the
   disk.  Each bit that corresponds to a sector within a file is
   set to true, and the other bits, which are not part of any
   file, are set to false.

   The second special file is the root directory, whose header is
   always stored in sector 1 (ROOT_DIR_SECTOR).  The root
   directory file stores an array of `struct dir_entry' (see
   directory.h), each of which, if it is in use, associates a
   filename with the sector of the file's header.

   The filesystem implemented here has the following limitations:

     - No synchronization.  Concurrent accesses will interfere
       with one another.

     - File size is fixed at creation time.  Because the root
       directory is represented as a file, the number of files
       that may be created is also limited.

     - No indirect blocks.  This limits maximum file size to the
       number of sector pointers that fit in a single sector
       times the size of a sector, or 126 * 512 == 63 kB given
       32-bit sizes and 512-byte sectors.

     - No nested subdirectories.

     - Filenames limited to 14 characters.

     - A system crash mid-operation may corrupt the disk in a way
       that cannot be repaired automatically.  No `fsck' tool is
       provided in any case.

   Note: for the purposes of the "user processes" assignment
   (project 2), please treat all the code in the filesys
   directory as a black box.  No changes should be needed.  For
   that project, a single lock external to the filesystem code
   suffices. */

/* File header sectors for system files. */
#define FREE_MAP_SECTOR 0       /* Free map file header sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file header sector. */

/* Root directory. */
#define NUM_DIR_ENTRIES 10      /* Maximum number of directory entries. */
#define ROOT_DIR_FILE_SIZE      /* Root directory file size in bytes. */ \
         (sizeof (struct dir_entry) * NUM_DIR_ENTRIES)

/* The disk that contains the filesystem. */
struct disk *filesys_disk;

/* The free map and root directory files.
   These files are opened by filesys_init() and never closed. */
static struct file free_map_file, root_dir_file;

static void do_format (void);

/* Initializes the filesystem module.
   If FORMAT is true, reformats the filesystem. */
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

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
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

/* Opens a file named NAME and initializes FILE for usage with
   the file_*() functions declared in file.h.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
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

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
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

/* Prints a list of files in the filesystem to the system
   console.
   Returns true if successful, false on failure,
   which occurs only if an internal memory allocation fails. */
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

/* Dumps the filesystem state to the system console,
   including the free map, the list of files, and file contents.
   Returns true if successful, false on failure,
   which occurs only if an internal memory allocation fails. */
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

static void must_succeed_function (int, bool) NO_INLINE;
#define MUST_SUCCEED(EXPR) must_succeed_function (__LINE__, EXPR)

/* Performs basic sanity checks on the filesystem.
   The filesystem should not contain a file named `foo' when
   called. */
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

/* Formats the filesystem. */
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

/* If SUCCESS is false, panics with an error complaining about
   LINE_NO. */
static void 
must_succeed_function (int line_no, bool success) 
{
  if (!success)
    PANIC ("filesys_self_test: operation failed on line %d", line_no);
}
