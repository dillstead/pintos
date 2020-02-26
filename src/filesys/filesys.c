#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/buffers.h"
#include "filesys/path.h"
#include "threads/thread.h"
#include "devices/input.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  struct inode *dinode;
  
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  buffers_init ();
  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();

  /* Create the initial links in the root directory. */
  dinode = inode_open (ROOT_DIR_SECTOR);
  inode_lock (dinode);
  dir_add (dinode, ".", ROOT_DIR_SECTOR);
  dir_add (dinode, "..", ROOT_DIR_SECTOR);
  inode_unlock (dinode);
  thread_current ()->cwd = dinode;
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  buffers_done ();
  free_map_close ();
}

/* Creates a file named PATH with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file or directory  named PATH already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *path, off_t initial_size) 
{
  char name[NAME_MAX + 1];
  struct inode *dinode;
  block_sector_t inode_sector = 0;
  bool success = false;

  if (path_is_empty (path) || path_has_trailing_slash (path))
    return false;
  dinode = path_lookup_parent (path, name);
  if (dinode == NULL)
    return false;
  inode_lock (dinode);
  if (!free_map_allocate (1, &inode_sector)
      || !inode_create (inode_sector, initial_size, false)
      || !dir_add (dinode, name, inode_sector))
    goto done;
  success = true;

 done:
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);
  inode_unlock (dinode);
  inode_close (dinode);
  return success;
}

/* Opens a file or directory with the given PATH.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file or directory named PATH exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *path)
{
  struct inode *inode;

  if (path_is_empty (path))
    return NULL;
  inode = path_lookup (path);
  if (inode == NULL)
    return NULL;
  if (path_has_trailing_slash (path) && !inode_is_dir (inode))
    {
      inode_close (inode);
      return NULL;
    }
  return file_open (inode);
}

/* Deletes a file or directory named PATH.
   Returns true if successful, false on failure.
   Fails if no file or directory named PATH exists,
   if the directory being deleted is not empty,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *path) 
{
  char name[NAME_MAX + 1];
  struct inode *dinode;
  struct inode *inode = NULL;
  bool success = false;
  bool is_dir;

  if (path_is_empty (path))
    return false;
  dinode = path_lookup_parent (path, name);
  if (dinode == NULL)
    return false;
  if (strcmp (name, ".") == 0 || strcmp (name, "..") == 0)
    {
      inode_close (dinode);
      return false;
    }
  inode_lock (dinode);
  if (!dir_lookup (dinode, name, &inode))
    goto done;
  is_dir = inode_is_dir (inode);
  if (is_dir)
    {
      /* Checking to see if the directory to be removed is empty must happen 
         atomically to prevent another process from adding a directory after 
         the empty check.  In order to do this two locks need to be acquired,
         a lock for the parent and for the directory.  This will not lead to
         deadlock because locks are always acquired in the same order. */
      inode_lock (inode);
      if (!dir_is_empty (inode))
        {
          inode_unlock (inode);
          goto done;
        }
    }
  else if (path_has_trailing_slash (path))
    goto done;
  dir_remove (dinode, name);
  /* Remove a directory with the lock held in case another thread is reading the
     deleted directory. */
  inode_remove (inode);
  if (is_dir)
    inode_unlock (inode);
  success = true;
  
 done:
  if (inode != NULL)
    inode_close (inode);
  inode_unlock (dinode);
  inode_close (dinode);
  return success;
}

static int
allocate_fd (struct file *file)
{
  struct thread *cur = thread_current ();
  int fd = -1;

  /* Allocate the first available file descriptor. */
  for (fd = 2; fd < MAX_OPEN_FILES; fd++)
    if (cur->ofiles[fd] == NULL)
      break;
  if (fd < MAX_OPEN_FILES)
    cur->ofiles[fd] = file;

  return fd;
}

/* The following functions provide a file descriptor wrapper around 
   the file system functionality. */
int
fd_open (const char *name, bool deny_write)
{
  struct file *file;
  int fd = -1;

  file = filesys_open (name);
  if (file != NULL && deny_write)
    file_deny_write (file);

  if (file != NULL)
    {
      fd = allocate_fd (file);
      if (fd == -1)
        file_close (file);
    }
  return fd;
}

off_t
fd_size (int fd)
{
  struct file *file;
  off_t size = -1;

  file = fd_get_file (fd);
  if (file != NULL && !file_is_dir (file))
    size = file_length (file);
  return size;
}

off_t
fd_read (int fd, void *buffer_, off_t size)
{
  uint8_t *buffer = buffer_;
  struct file *file;
  off_t bytes_read = -1;
  uint8_t c;

  if (fd == STDIN_FILENO)
    {
      /* Treat stdin as a special file descriptor that reads lines from the 
         keyboard. */
      while (bytes_read < size)
        {
          c = input_getc ();
          if (c == '\n')
            break;
          buffer[bytes_read++] = c;
        }
    }
  else
    {
      file = fd_get_file (fd);
      if (file != NULL && !file_is_dir (file))
        bytes_read = file_read (file, buffer, size);
    }
  return bytes_read;
}

off_t
fd_write (int fd, const void *buffer, off_t size)
{
  struct file *file;
  off_t bytes_written = -1;

  if (fd == STDOUT_FILENO)
    {
      /* Treat stdout as a special file descriptor that writes to the console. */
      putbuf (buffer, size);
      bytes_written = size;
    }
  else
    {
      file = fd_get_file (fd);
      if (file != NULL && !file_is_dir (file))
        bytes_written = file_write (file, buffer, size);
    }
  return bytes_written;
}

void
fd_seek (int fd, off_t new_pos)
{
  struct file *file;

  file = fd_get_file (fd);
  if (file != NULL && !file_is_dir (file))
    {
      file_seek (file, new_pos);
    }
}

off_t
fd_tell (int fd)
{
  struct file *file;
  off_t pos = -1;

  file = fd_get_file (fd);
  if (file != NULL && !file_is_dir (file))
    {
      pos = file_tell (file);
    }
  return pos;
}

void
fd_close (int fd)
{
  struct thread *cur = thread_current ();
  struct file *file;

  file = fd_get_file (fd);
  if (file != NULL)
    {
      file_allow_write (file);
      file_close (file);
      cur->ofiles[fd] = NULL;
    }
}

/* Changes the current directory of a process to PATH. */
bool
filesys_chdir (const char *path)
{
  struct thread *cur = thread_current ();
  struct inode *inode;

  if (path_is_empty (path))
    return false;
  inode = path_lookup (path);
  if (inode == NULL)
    return false;
  inode_close (cur->cwd);
  cur->cwd = inode;
  return true;
}

/* Creates a directory named PATH with enough space
   for INITIAL_SIZE directory entries.
   Returns true if successful, false otherwise.
   Fails if a file or directory named PATH already exists,
   or if internal memory allocation fails. */
bool
filesys_mkdir (const char *path, off_t initial_size)
{
  char name[NAME_MAX + 1];
  struct inode *dinode;
  struct inode *inode = NULL;
  block_sector_t inode_sector = 0;
  bool success = false;

  if (path_is_empty (path))
    return false;
  dinode = path_lookup_parent (path, name);
  if (dinode == NULL)
    return false;
  inode_lock (dinode);
  if (!free_map_allocate (1, &inode_sector)
      || !inode_create (inode_sector, initial_size, true))
    goto done;
  inode = inode_open (inode_sector);
  if (inode == NULL)
    goto done;
  /* pAdd "." and ".." to the directory. */
  if (!dir_add (inode, ".", inode_sector)
      || !dir_add (inode, "..", inode_get_inumber (dinode))
      || !dir_add (dinode, name, inode_sector))
    goto done;
  success = true;

 done:
  if (!success)
    {
      if (inode != NULL)
        inode_remove (inode);
      else if (inode_sector != 0)
        free_map_release (inode_sector, 1);
    }
  if (inode != NULL)
    inode_close (inode);
  inode_unlock (dinode);
  inode_close (dinode);
  return success;
}

bool
fd_readdir (int fd, char *name)
{
  struct file *file;
  bool success = false;

  file = fd_get_file (fd);
  if (file != NULL && file_is_dir (file))
    success = dir_readdir (file, name);
  return success;
}

bool
fd_is_dir (int fd)
{
  struct file *file;
  bool is_dir = false;

  file = fd_get_file (fd);
  if (file != NULL)
    is_dir = file_is_dir (file);
  return is_dir;
}

block_sector_t
fd_inumber (int fd)
{
  struct file *file;
  block_sector_t inumber = 0;

  file = fd_get_file (fd);
  if (file != NULL)
    inumber = inode_get_inumber (file_get_inode (file));
  return inumber;
}

struct file *
fd_get_file (int fd)
{
  struct thread *cur = thread_current ();
  
  if (fd < 2 || fd >= MAX_OPEN_FILES)
    return NULL;
  return cur->ofiles[fd];
}


/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

