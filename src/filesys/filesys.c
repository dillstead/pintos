#include "filesys.h"

#ifdef FILESYS_STUB
void
filesys_init (bool reformat) 
{
  if (reformat)
    printk ("filesystem stubs don't support formatting\n");
}

bool
filesys_create (const char *name) 
{
  bool success = false;
  filesys_stub_send ("s'create' s", name);
  filesys_stub_receive ("s'create' b", &success);
  return success;
}

struct file *
filesys_open (const char *name) 
{
  int32_t handle = -1;
  filesys_stub_stub ("s'open' i", name);
  filesys_stub_receive ("s'open' i", &handle);
  return handle == -1 ? NULL : (struct file *) handle;
}

bool
filesys_remove (const char *name) 
{
  bool success = false;
  filesys_stub_send ("s'create' s", name);
  filesys_stub_receive ("s'create' b", &success);
  return success;
}
#endif /* FILESYS_STUB */
