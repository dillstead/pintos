#include <random.h>
#include <syscall.h>
#include "fslib.h"

const char test_name[] = "grow-two-files";

#define FILE_SIZE 8143
static char buf_a[FILE_SIZE];
static char buf_b[FILE_SIZE];

static void
write_some_bytes (const char *filename, int fd, const char *buf, size_t *ofs) 
{
  if (*ofs < FILE_SIZE) 
    {
      size_t block_size = random_ulong () % (FILE_SIZE / 8) + 1;
      if (block_size > FILE_SIZE - *ofs)
        block_size = FILE_SIZE - *ofs;
  
      if (write (fd, buf + *ofs, block_size) <= 0)
        fail ("write %zu bytes at offset %zu in \"%s\"",
              block_size, *ofs, filename);
      *ofs += block_size;
    }
}

int
main (void) 
{
  int fd_a, fd_b;
  size_t ofs_a, ofs_b;

  random_init (0);
  random_bytes (buf_a, sizeof buf_a);
  random_bytes (buf_b, sizeof buf_b);

  check (create ("a", 0), "create \"a\"");
  check (create ("b", 0), "create \"b\"");

  check ((fd_a = open ("a")) > 1, "open \"a\"");
  check ((fd_b = open ("b")) > 1, "open \"b\"");

  msg ("write \"a\" and \"b\" alternately");
  while (ofs_a < FILE_SIZE || ofs_b < FILE_SIZE) 
    {
      write_some_bytes ("a", fd_a, buf_a, &ofs_a);
      write_some_bytes ("b", fd_b, buf_b, &ofs_b);
    }

  msg ("close \"a\"");
  close (fd_a);

  msg ("close \"b\"");
  close (fd_b);

  check_file ("a", buf_a, FILE_SIZE);
  check_file ("b", buf_b, FILE_SIZE);

  return 0;
}
