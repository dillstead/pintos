#include "file.h"

#ifdef FILESYS_STUB
off_t
file_read (struct file *file, void *buffer, off_t size) 
{
  int32_t retval = -1;
  filesys_stub_send ("s'read' i i", (int32_t) file, (int32_t) size);
  filesys_stub_receive ("s'read' i", &retval);
  if (retval > 0) 
    {
      if (!filesys_stub_receive ("B", buffer, (size_t) retval))
        retval = -1;
    }
  return retval;
}

off_t
file_write (struct file *file, const void *buffer, off_t size) 
{
  int32_t retval = -1;
  filesys_stub_send ("s'write' i B", (int32_t) file, buffer, (size_t) size);
  filesys_stub_receive ("s'write' i", &retval);
  return retval;
}

off_t
file_length (struct file *file) 
{
  int32_t length = -1;
  filesys_stub_send ("s'length' i", (int32_t) file);
  filesys_stub_receive ("s'length' i", &length);
  return length;
}

void
file_seek (struct file *file, off_t pos) 
{
  filesys_stub_send ("s'seek' i i", (int32_t) file, (int32_t) pos);
  filesys_stub_receive ("s'seek'");
}

off_t
file_tell (struct file *file) 
{
  int32_t pos = -1;
  filesys_stub_send ("s'tell' i", (int32_t) file);
  filesys_stub_receive ("s'tell'", &pos);
  return pos;
}
#endif /* FILESYS_STUB */
