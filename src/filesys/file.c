#include "file.h"

#ifdef FILESYS_STUB
#include "debug.h"
#include "filesys-stub.h"
#include "lib.h"
#include "malloc.h"

void
file_close (struct file *file) 
{
  filesys_stub_lock ();
  filesys_stub_put_string ("close");
  filesys_stub_put_file (file);
  filesys_stub_match_string ("close");
  filesys_stub_unlock ();
}

off_t
file_read (struct file *file, void *buffer, off_t size) 
{
  int32_t retval;

  filesys_stub_lock ();
  filesys_stub_put_string ("read");
  filesys_stub_put_file (file);
  filesys_stub_put_uint32 (size);
  filesys_stub_match_string ("read");
  retval = filesys_stub_get_int32 ();
  if (retval > 0) 
    {
      ASSERT (retval <= size);
      filesys_stub_get_bytes (buffer, retval);
    }
  filesys_stub_unlock ();
  
  return retval;
}

off_t
file_write (struct file *file, const void *buffer, off_t size) 
{
  int32_t retval;

  filesys_stub_lock ();
  filesys_stub_put_string ("write");
  filesys_stub_put_file (file);
  filesys_stub_put_uint32 (size);
  filesys_stub_put_bytes (buffer, size);
  filesys_stub_match_string ("write");
  retval = filesys_stub_get_int32 ();
  ASSERT (retval <= size);
  filesys_stub_unlock ();

  return retval;
}

off_t
file_length (struct file *file) 
{
  int32_t length;

  filesys_stub_lock ();
  filesys_stub_put_string ("length");
  filesys_stub_put_file (file);
  filesys_stub_match_string ("length");
  length = filesys_stub_get_int32 ();
  filesys_stub_unlock ();

  return length;
}

void
file_seek (struct file *file, off_t pos) 
{
  filesys_stub_lock ();
  filesys_stub_put_string ("seek");
  filesys_stub_put_file (file);
  filesys_stub_put_uint32 (pos);
  filesys_stub_match_string ("seek");
  filesys_stub_unlock ();
}

off_t
file_tell (struct file *file) 
{
  int32_t pos;

  filesys_stub_lock ();
  filesys_stub_put_string ("tell");
  filesys_stub_put_file (file);
  filesys_stub_match_string ("tell");
  pos = filesys_stub_get_int32 ();
  filesys_stub_unlock ();

  return pos;
}
#endif /* FILESYS_STUB */
