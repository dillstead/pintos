#ifndef FSLIB_H
#define FSLIB_H

#include <debug.h>
#include <stdbool.h>
#include <stddef.h>

extern const char test_name[];
extern bool quiet;

void msg (const char *, ...) PRINTF_FORMAT (1, 2);
void fail (const char *, ...) PRINTF_FORMAT (1, 2) NO_RETURN;

#define check(SUCCESS, ...)                     \
        do                                      \
          {                                     \
            msg (__VA_ARGS__);                  \
            if (!(SUCCESS))                     \
              fail (__VA_ARGS__);               \
          }                                     \
        while (0)

void shuffle (void *, size_t cnt, size_t size);

void seq_test (const char *filename,
               void *buf, size_t size, size_t initial_size,
               size_t (*block_size_func) (void),
               void (*check_func) (int fd, long ofs));

void check_file (const char *filename, const void *buf, size_t filesize);

void compare_bytes (const void *read_data, const void *expected_data,
                    size_t size, size_t ofs, const char *filename);

void test_main (void);

#endif /* fslib.h */
