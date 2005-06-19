#include "tests/filesys/seq-test.h"
#include <random.h>
#include <syscall.h>
#include "tests/lib.h"

void 
seq_test (const char *filename, void *buf, size_t size, size_t initial_size,
          size_t (*block_size_func) (void),
          void (*check_func) (int fd, long ofs)) 
{
  size_t ofs;
  int fd;
  
  random_bytes (buf, size);
  CHECK (create (filename, initial_size), "create \"%s\"", filename);
  CHECK ((fd = open (filename)) > 1, "open \"%s\"", filename);

  ofs = 0;
  msg ("writing \"%s\"", filename);
  while (ofs < size) 
    {
      size_t block_size = block_size_func ();
      if (block_size > size - ofs)
        block_size = size - ofs;

      if (write (fd, buf + ofs, block_size) != (int) block_size)
        fail ("write %zu bytes at offset %zu in \"%s\" failed",
              block_size, ofs, filename);

      ofs += block_size;
      if (check_func != NULL)
        check_func (fd, ofs);
    }
  msg ("close \"%s\"", filename);
  close (fd);
  check_file (filename, buf, size);
}
