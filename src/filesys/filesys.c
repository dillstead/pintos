#include "filesys.h"


#ifdef FILESYS_STUB
#include <stdint.h>
#include "debug.h"
#include "filesys-stub.h"
#include "lib.h"

void
filesys_init (bool reformat) 
{
  if (reformat)
    printk ("filesystem stubs don't support formatting\n");
  filesys_stub_init ();
}

bool
filesys_create (const char *name) 
{
  bool success;

  filesys_stub_lock ();
  filesys_stub_put_string ("create");
  filesys_stub_put_string (name);
  filesys_stub_match_string ("create");
  success = filesys_stub_get_bool ();
  filesys_stub_unlock ();

  return success;
}

struct file *
filesys_open (const char *name) 
{
  struct file *file;

  filesys_stub_lock ();
  filesys_stub_put_string ("open");
  filesys_stub_put_string (name);
  filesys_stub_match_string ("open");
  file = filesys_stub_get_file ();
  filesys_stub_unlock ();
  
  return file;
}

bool
filesys_remove (const char *name) 
{
  bool success;

  filesys_stub_lock ();
  filesys_stub_put_string ("remove");
  filesys_stub_put_string (name);
  filesys_stub_match_string ("remove");
  success = filesys_stub_get_bool ();
  filesys_stub_unlock ();

  return success;
}
#endif /* FILESYS_STUB */

#undef NDEBUG
#include "debug.h"
#include "file.h"

void
filesys_self_test (void)
{
  static const char s[] = "This is a test string.";
  struct file *file;
  char s2[sizeof s];

  ASSERT (filesys_create ("foo"));
  ASSERT ((file = filesys_open ("foo")) != NULL);
  ASSERT (file_write (file, s, sizeof s) == sizeof s);
  ASSERT (file_tell (file) == sizeof s);
  ASSERT (file_length (file) == sizeof s);
  file_close (file);

  ASSERT ((file = filesys_open ("foo")) != NULL);
  ASSERT (file_read (file, s2, sizeof s2) == sizeof s2);
  ASSERT (memcmp (s, s2, sizeof s) == 0);
  ASSERT (file_tell (file) == sizeof s2);
  ASSERT (file_length (file) == sizeof s2);
  file_close (file);

  ASSERT (filesys_remove ("foo"));
}
