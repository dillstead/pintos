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

#include "filesys/filesys.h"
#include <bitmap.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/disk.h"

/* Filesystem.

   For the purposes of the "user processes" assignment (project
   2), please treat all the code in the filesys directory as a
   black box.  No changes should be needed.  For that project, a
   single lock external to the filesystem code suffices.

   The filesystem consists of a set of files.  Each file has a
   header called an `index node' or `inode', represented by
   struct inode, that is stored by itself in a single sector (see
   inode.h).  The header contains the file's length in bytes and
   an array that lists the sector numbers for the file's
   contents.

   Two files are special.  The first special file is the free
   map, whose inode is always stored in sector 0
   (FREE_MAP_SECTOR).  The free map stores a bitmap (see
   lib/bitmap.h) that contains one bit for each sector on the
   disk.  Each bit that corresponds to a sector within a file is
   set to true, and the other bits, which are not part of any
   file, are set to false.

   The second special file is the root directory, whose inode is
   always stored in sector 1 (ROOT_DIR_SECTOR).  The root
   directory file stores an array of `struct dir_entry' (see
   directory.h), each of which, if it is in use, associates a
   filename with the sector of the file's inode.

   The filesystem implemented here has the following limitations:

     - No synchronization.  Concurrent accesses will interfere
       with one another, so external synchronization is needed.

     - File size is fixed at creation time.  Because the root
       directory is represented as a file, the number of files
       that may be created is also limited.

     - No indirect blocks.  This limits maximum file size to the
       number of sector pointers that fit in a single inode
       times the size of a sector, or 126 * 512 == 63 kB given
       32-bit sizes and 512-byte sectors.

     - No subdirectories.

     - Filenames limited to 14 characters.

     - A system crash mid-operation may corrupt the disk in a way
       that cannot be repaired automatically.  No `fsck' tool is
       provided in any case.

   However one important feature is included:

     - Unix-like semantics for filesys_remove() are implemented.
       That is, if a file is open when it is removed, its blocks
       are not deallocated and it may still be accessed by the
       threads that have it open until the last one closes it. */

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */

/* Root directory. */
#define NUM_DIR_ENTRIES 10      /* Maximum number of directory entries. */

/* The disk that contains the filesystem. */
struct disk *filesys_disk;

/* The free map and root directory files.
   These files are opened by filesys_init() and never closed. */
struct file *free_map_file, *root_dir_file;

static void do_format (void);

/* Initializes the filesystem module.
   If FORMAT is true, reformats the filesystem. */
void
filesys_init (bool format) 
{
  inode_init ();

  filesys_disk = disk_get (0, 1);
  if (filesys_disk == NULL)
    PANIC ("hd0:1 (hdb) not present, filesystem initialization failed");

  if (format) 
    do_format ();
  
  free_map_file = file_open (FREE_MAP_SECTOR);
  if (free_map_file == NULL)
    PANIC ("can't open free map file");
  root_dir_file = file_open (ROOT_DIR_SECTOR);
  if (root_dir_file == NULL)
    PANIC ("can't open root dir file");
}

/* Shuts down the filesystem module, writing any unwritten data
   to disk.
   Currently there's nothing to do.  You'll need to add code here
   when you implement write-behind caching. */
void
filesys_done (void) 
{
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  struct dir *dir = NULL;
  struct bitmap *free_map = NULL;
  struct inode *inode = NULL;
  disk_sector_t inode_sector;
  bool success = false;

  /* Read the root directory. */
  dir = dir_create (NUM_DIR_ENTRIES);
  if (dir == NULL)
    goto done;
  dir_read (dir, root_dir_file);
  if (dir_lookup (dir, name, NULL)) 
    goto done;

  /* Allocate a block for the inode. */
  free_map = bitmap_create (disk_size (filesys_disk));
  if (free_map == NULL)
    goto done;
  bitmap_read (free_map, free_map_file);
  inode_sector = bitmap_scan_and_flip (free_map, 0, 1, false);
  if (inode_sector == BITMAP_ERROR)
    goto done;

  /* Add the file to the directory. */
  if (!dir_add (dir, name, inode_sector))
    goto done;

  /* Allocate space for the file. */
  inode = inode_create (free_map, inode_sector, initial_size);
  if (inode == NULL)
    goto done;

  /* Write everything back. */
  inode_commit (inode);
  dir_write (dir, root_dir_file);
  bitmap_write (free_map, free_map_file);

  success = true;

  /* Clean up. */
 done:
  inode_close (inode);
  bitmap_destroy (free_map);
  dir_destroy (dir);

  return success;
}

/* Opens a file named NAME and initializes FILE for usage with
   the file_*() functions declared in file.h.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = NULL;
  struct file *file = NULL;
  disk_sector_t inode_sector;

  dir = dir_create (NUM_DIR_ENTRIES);
  if (dir == NULL)
    goto done;

  dir_read (dir, root_dir_file);
  if (dir_lookup (dir, name, &inode_sector))
    file = file_open (inode_sector);

 done:
  dir_destroy (dir); 

  return file;
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = NULL;
  struct inode *inode;
  disk_sector_t inode_sector;
  bool success = false;

  /* Read the root directory. */
  dir = dir_create (NUM_DIR_ENTRIES);
  if (dir == NULL)
    goto done;
  dir_read (dir, root_dir_file);
  if (!dir_lookup (dir, name, &inode_sector))
    goto done;

  /* Open the inode and delete it it. */
  inode = inode_open (inode_sector);
  if (inode == NULL)
    goto done;
  inode_remove (inode);
  inode_close (inode);

  /* Remove file from root directory and write directory back to
     disk. */
  dir_remove (dir, name);
  dir_write (dir, root_dir_file);

  success = true;

  /* Clean up. */
 done:
  dir_destroy (dir);

  return success;
}

/* Prints a list of files in the filesystem to the system
   console.
   Returns true if successful, false on failure,
   which occurs only if an internal memory allocation fails. */
bool
filesys_list (void) 
{
  struct dir *dir = dir_create (NUM_DIR_ENTRIES);
  if (dir == NULL)
    return false;
  dir_read (dir, root_dir_file);
  dir_list (dir);
  dir_destroy (dir);

  return true;
}

/* Dumps the filesystem state to the system console,
   including the free map, the list of files, and file contents.
   Returns true if successful, false on failure,
   which occurs only if an internal memory allocation fails. */
bool
filesys_dump (void) 
{
  struct bitmap *free_map;
  struct dir *dir;  

  printf ("Free map:\n");
  free_map = bitmap_create (disk_size (filesys_disk));
  if (free_map == NULL)
    return false;
  bitmap_read (free_map, free_map_file);
  bitmap_dump (free_map);
  bitmap_destroy (free_map);
  printf ("\n");
  
  dir = dir_create (NUM_DIR_ENTRIES);
  if (dir == NULL)
    return false;
  dir_read (dir, root_dir_file);
  dir_dump (dir);
  dir_destroy (dir);

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
      MUST_SUCCEED (file_read (file, s2, sizeof s) == sizeof s);
      MUST_SUCCEED (memcmp (s, s2, sizeof s) == 0);
      MUST_SUCCEED (file_tell (file) == sizeof s);
      MUST_SUCCEED (file_length (file) == sizeof s);
      file_close (file);

      /* Make sure file is deleted. */
      MUST_SUCCEED ((file = filesys_open ("foo")) == NULL);
    }
  
  printf ("filesys: self test ok\n");
}

/* Formats the filesystem. */
static void
do_format (void)
{
  struct bitmap *free_map;
  struct inode *map_inode, *dir_inode;
  struct dir *dir;

  printf ("Formatting filesystem...");

  /* Create the initial bitmap and reserve sectors for the
     free map and root directory inodes. */
  free_map = bitmap_create (disk_size (filesys_disk));
  if (free_map == NULL)
    PANIC ("bitmap creation failed--disk is too large");
  bitmap_mark (free_map, FREE_MAP_SECTOR);
  bitmap_mark (free_map, ROOT_DIR_SECTOR);

  /* Allocate data sector(s) for the free map file
     and write its inode to disk. */
  map_inode = inode_create (free_map, FREE_MAP_SECTOR,
                            bitmap_file_size (free_map));
  if (map_inode == NULL)
    PANIC ("free map creation failed--disk is too large");
  inode_commit (map_inode);
  inode_close (map_inode);

  /* Allocate data sector(s) for the root directory file
     and write its inodes to disk. */
  dir_inode = inode_create (free_map, ROOT_DIR_SECTOR,
                            dir_size (NUM_DIR_ENTRIES));
  if (dir_inode == NULL)
    PANIC ("root directory creation failed");
  inode_commit (dir_inode);
  inode_close (dir_inode);

  /* Write out the free map now that we have space reserved
     for it. */
  free_map_file = file_open (FREE_MAP_SECTOR);
  if (free_map_file == NULL)
    PANIC ("can't open free map file");
  bitmap_write (free_map, free_map_file);
  bitmap_destroy (free_map);
  file_close (free_map_file);

  /* Write out the root directory in the same way. */
  root_dir_file = file_open (ROOT_DIR_SECTOR);
  if (root_dir_file == NULL)
    PANIC ("can't open root directory");
  dir = dir_create (NUM_DIR_ENTRIES);
  if (dir == NULL)
    PANIC ("can't initialize root directory");
  dir_write (dir, root_dir_file);
  dir_destroy (dir);
  file_close (root_dir_file);

  printf ("done.\n");
}

/* If SUCCESS is false, panics with an error complaining about
   LINE_NO. */
static void 
must_succeed_function (int line_no, bool success) 
{
  if (!success)
    PANIC ("filesys_self_test: operation failed on line %d", line_no);
}
