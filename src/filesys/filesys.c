#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"
#include "devices/input.h"

/* Serialize all process file system operations since they are 
   not thread safe. */
static struct lock filesys_lock;

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  lock_init (&filesys_lock);
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
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
  block_sector_t inode_sector = 0;
  struct dir *dir = dir_open_root ();
  bool success = (dir != NULL
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
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

  if (dir != NULL)
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
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 

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

static struct file *
get_file (int fd)
{
  struct thread *cur = thread_current ();
    
  if (fd < 2 || fd >= MAX_OPEN_FILES)
    return NULL;
  
  return cur->ofiles[fd];
}

/* The following functions provide a file descriptor wrapper around 
   the file system functionality.  The file system is not thread safe
   so the underlying file system calls are locked to prevent simultaneous
   use. */
bool
process_file_create (const char *name, off_t initial_size)
{
  bool success;
  
  lock_acquire (&filesys_lock);
  success = filesys_create (name, initial_size);
  lock_release (&filesys_lock);
  
  return success;
}

bool
process_file_remove (const char *name)
{
  bool success;
  
  lock_acquire (&filesys_lock);
  success = filesys_remove (name);
  lock_release (&filesys_lock);
  
  return success;
}

int
process_file_open (const char *name)
{
  struct file *file;
  int fd = -1;
  
  lock_acquire (&filesys_lock);
  file = filesys_open (name);
  lock_release (&filesys_lock);

  if (file != NULL)
    {
      fd = allocate_fd (file);
      if (fd == -1)
        {
          lock_acquire (&filesys_lock);
          file_close (file);
          lock_release (&filesys_lock);
        }
    }
  
  return fd;
}

off_t
process_file_size (int fd)
{
  struct file *file;
  off_t size = 0;

  file = get_file (fd);
  if (file != NULL)
    {
      lock_acquire (&filesys_lock);
      size = file_length (file);
      lock_release (&filesys_lock);
    }
  
  return size;
}

off_t
process_file_read (int fd, void *buffer_, off_t size)
{
  uint8_t *buffer = buffer_;
  struct file *file;
  off_t bytes_read = 0;
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
      file = get_file (fd);
      if (file != NULL)
        {
          lock_acquire (&filesys_lock);
          bytes_read = file_read (file, buffer, size);
          lock_release (&filesys_lock);
        }
    }

  return bytes_read;
}

off_t
process_file_write (int fd, const void *buffer, off_t size)
{
  struct file *file;
  off_t bytes_written = 0;
  
  if (fd == STDOUT_FILENO)
    {
      /* Treat stdout as a special file descriptor that writes to the console. */
      putbuf (buffer, size);
      bytes_written = size;
    }
  else
    {
      file = get_file (fd);
      if (file != NULL)
        {
          lock_acquire (&filesys_lock);
          bytes_written = file_write (file, buffer, size);
          lock_release (&filesys_lock);
        }
    }
    
  return bytes_written;
}

void
process_file_seek (int fd, off_t new_pos)
{
  struct file *file;

  file = get_file (fd);
  if (file != NULL)
    {
      lock_acquire (&filesys_lock);
      file_seek (file, new_pos);
      lock_release (&filesys_lock);
    }
}

off_t
process_file_tell (int fd)
{
  struct file *file;
  off_t pos = 0;

  file = get_file (fd);
  if (file != NULL)
    {
      lock_acquire (&filesys_lock);
      pos = file_tell (file);
      lock_release (&filesys_lock);
    }
  
  return pos;
}

void
process_file_close (int fd)
{
  struct thread *cur = thread_current ();
  struct file *file;

  file = get_file (fd);
  if (file != NULL)
    {
      lock_acquire (&filesys_lock);
      file_close (file);
      lock_release (&filesys_lock);
      cur->ofiles[fd] = NULL;
    }
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
