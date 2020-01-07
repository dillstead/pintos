#include "filesys/file.h"
#ifdef DEBUG_FILE
#include <stdio.h>
#endif
#include <debug.h>
#include "filesys/inode.h"
#include "threads/malloc.h"
#ifdef DEBUG_FILE
#include "threads/thread.h"
#endif

/* An open file. */
struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };

/* Opens a file for the given INODE, of which it takes ownership,
   and returns the new file.  Returns a null pointer if an
   allocation fails or if INODE is null. */
struct file *
file_open (struct inode *inode) 
{
#ifdef DEBUG_FILE
  printf ("file_open enter %s%d, inode: %p\n", thread_name (),
          thread_current ()->tid, inode);
#endif
  struct file *file = calloc (1, sizeof *file);
  if (inode != NULL && file != NULL)
    {
      file->inode = inode;
      file->pos = 0;
      file->deny_write = false;
#ifdef DEBUG_FILE
  printf ("file_open exit %s%d, file: %p\n", thread_name (),
          thread_current ()->tid, file);
#endif
      return file;
    }
  else
    {
      inode_close (inode);
      free (file);
#ifdef DEBUG_FILE
  printf ("file_open exit %s%d, file: NULL\n", thread_name (),
          thread_current ()->tid);
#endif      
      return NULL; 
    }
}

/* Opens and returns a new file for the same inode as FILE.
   Returns a null pointer if unsuccessful. */
struct file *
file_reopen (struct file *file) 
{
  struct file *refile;
#ifdef DEBUG_FILE
  printf ("file_reopen enter %s%d, file: %p\n", thread_name (),
          thread_current ()->tid, file);
#endif
  refile = file_open (inode_reopen (file->inode));
#ifdef DEBUG_FILE
  printf ("file_reopen exit %s%d, file: %p\n", thread_name (),
          thread_current ()->tid, refile);
#endif
  return refile;
}

/* Closes FILE. */
void
file_close (struct file *file) 
{
#ifdef DEBUG_FILE
  printf ("file_close enter %s%d, file: %p\n", thread_name (),
          thread_current ()->tid, file);
#endif
  if (file != NULL)
    {
      file_allow_write (file);
      inode_close (file->inode);
      free (file); 
    }
#ifdef DEBUG_FILE
  printf ("file_close exit %s%d\n", thread_name (), thread_current ()->tid);
#endif
}

/* Returns the inode encapsulated by FILE. */
struct inode *
file_get_inode (struct file *file) 
{
  return file->inode;
}

/* Reads SIZE bytes from FILE into BUFFER,
   starting at the file's current position.
   Returns the number of bytes actually read,
   which may be less than SIZE if end of file is reached.
   Advances FILE's position by the number of bytes read. */
off_t
file_read (struct file *file, void *buffer, off_t size) 
{
#ifdef DEBUG_FILE
  printf ("file_read enter %s%d, file: %p, sz: %u\n", thread_name (),
          thread_current ()->tid, file, size);
#endif
  off_t bytes_read = inode_read_at (file->inode, buffer, size, file->pos);
  file->pos += bytes_read;
#ifdef DEBUG_FILE
  printf ("file_read exit %s%d, bytes: %u\n", thread_name (),
          thread_current ()->tid, bytes_read);
#endif
  return bytes_read;
}

/* Reads SIZE bytes from FILE into BUFFER,
   starting at offset FILE_OFS in the file.
   Returns the number of bytes actually read,
   which may be less than SIZE if end of file is reached.
   The file's current position is unaffected. */
off_t
file_read_at (struct file *file, void *buffer, off_t size, off_t file_ofs) 
{
#ifdef DEBUG_FILE
  printf ("file_read_at enter %s%d, file: %p, sz: %u, off: %u\n",
          thread_name (), thread_current ()->tid, file, size, file_ofs);
#endif
  off_t bytes_read = inode_read_at (file->inode, buffer, size, file_ofs);
#ifdef DEBUG_FILE
  printf ("file_read_at exit %s%d, bytes: %u\n", thread_name (),
          thread_current ()->tid, bytes_read);
#endif
  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into FILE,
   starting at the file's current position.
   Returns the number of bytes actually written,
   which may be less than SIZE if end of file is reached.
   (Normally we'd grow the file in that case, but file growth is
   not yet implemented.)
   Advances FILE's position by the number of bytes read. */
off_t
file_write (struct file *file, const void *buffer, off_t size) 
{
#ifdef DEBUG_FILE
  printf ("file_write enter %s%d, file: %p, sz: %u\n", thread_name (),
          thread_current ()->tid, file, size);
#endif
  off_t bytes_written = inode_write_at (file->inode, buffer, size, file->pos);
  file->pos += bytes_written;
#ifdef DEBUG_FILE
  printf ("file_write exit %s%d, bytes: %u\n", thread_name (),
          thread_current ()->tid, bytes_written);
#endif  
  return bytes_written;
}

/* Writes SIZE bytes from BUFFER into FILE,
   starting at offset FILE_OFS in the file.
   Returns the number of bytes actually written,
   which may be less than SIZE if end of file is reached.
   (Normally we'd grow the file in that case, but file growth is
   not yet implemented.)
   The file's current position is unaffected. */
off_t
file_write_at (struct file *file, const void *buffer, off_t size,
               off_t file_ofs) 
{
#ifdef DEBUG_FILE
  printf ("file_write_at enter %s%d, file: %p, sz: %u, off: %u\n",
          thread_name (), thread_current ()->tid, file, size, file_ofs);
#endif
  off_t bytes_written = inode_write_at (file->inode, buffer, size, file_ofs);
#ifdef DEBUG_FILE
  printf ("file_write_at exit %s%d, bytes: %u\n", thread_name (),
          thread_current ()->tid, bytes_written);
#endif
  return bytes_written;
}

/* Prevents write operations on FILE's underlying inode
   until file_allow_write() is called or FILE is closed. */
void
file_deny_write (struct file *file) 
{
  ASSERT (file != NULL);
  if (!file->deny_write) 
    {
      file->deny_write = true;
      inode_deny_write (file->inode);
    }
}

/* Re-enables write operations on FILE's underlying inode.
   (Writes might still be denied by some other file that has the
   same inode open.) */
void
file_allow_write (struct file *file) 
{
  ASSERT (file != NULL);
  if (file->deny_write) 
    {
      file->deny_write = false;
      inode_allow_write (file->inode);
    }
}

/* Returns the size of FILE in bytes. */
off_t
file_length (struct file *file) 
{
  off_t length;
#ifdef DEBUG_FILE
  printf ("file_length enter %s%d, file: %p\n", thread_name (),
          thread_current ()->tid, file);
#endif
  ASSERT (file != NULL);
  length = inode_length (file->inode);
#ifdef DEBUG_FILE
  printf ("file_length exit %s%d, len: %u\n", thread_name (),
          thread_current ()->tid, length);
#endif
  return length;
}

/* Sets the current position in FILE to NEW_POS bytes from the
   start of the file. */
void
file_seek (struct file *file, off_t new_pos)
{
#ifdef DEBUG_FILE
  printf ("file_seek enter %s%d, file: %p, pos: %u\n", thread_name (),
          thread_current ()->tid, file, new_pos);
#endif
  ASSERT (file != NULL);
  ASSERT (new_pos >= 0);
  if (new_pos >= MAX_FILE_SIZE)
    new_pos = MAX_FILE_SIZE - 1;
  file->pos = new_pos;
#ifdef DEBUG_FILE
  printf ("file_seek exit %s%d\n", thread_name (), thread_current ()->tid);
#endif
}

/* Returns the current position in FILE as a byte offset from the
   start of the file. */
off_t
file_tell (struct file *file) 
{
#ifdef DEBUG_FILE
  printf ("file_tell enter %s%d, file: %p\n", thread_name (),
          thread_current ()->tid, file);
#endif
  off_t pos;
  ASSERT (file != NULL);
  pos = file->pos;
#ifdef DEBUG_FILE
  printf ("file_seek exit %s%d, pos: %u\n", thread_name (),
          thread_current ()->tid, pos);
#endif
  return pos;
}

/* Returns whether or not this FILE is a directory. */
bool
file_is_dir (struct file *file)
{
  return inode_is_dir (file->inode);
}
