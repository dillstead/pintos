#ifndef HEADER_IO_H
#define HEADER_IO_H 1

#include <stddef.h>
#include <stdint.h>

/* Reads and returns a byte from PORT. */
static inline uint8_t
inb (uint16_t port)
{
  uint8_t data;
  asm volatile ("inb %w1,%0" : "=a" (data) : "d" (port));
  return data;
}

/* Reads CNT bytes from PORT, one after another, and stores them
   into the buffer starting at ADDR. */
static inline void
insb (uint16_t port, void *addr, size_t cnt)
{
  asm volatile ("cld; repne; insb"
                : "=D" (addr), "=c" (cnt)
                : "d" (port), "0" (addr), "1" (cnt)
                : "memory", "cc");
}

/* Reads and returns 16 bits from PORT. */
static inline uint16_t
inw (uint16_t port)
{
  uint16_t data;
  asm volatile ("inw %w1,%0" : "=a" (data) : "d" (port));
  return data;
}

/* Reads CNT 16-bit (halfword) units from PORT, one after
   another, and stores them into the buffer starting at ADDR. */
static inline void
insw (uint16_t port, void *addr, size_t cnt)
{
  asm volatile ("cld; repne; insw"
                : "=D" (addr), "=c" (cnt)
                : "d" (port), "0" (addr), "1" (cnt)
                : "memory", "cc");
}

/* Reads and returns 32 bits from PORT. */
static inline uint32_t
inl (uint16_t port)
{
  uint32_t data;
  asm volatile ("inl %w1,%0" : "=a" (data) : "d" (port));
  return data;
}

/* Reads CNT 32-bit (word) units from PORT, one after another,
   and stores them into the buffer starting at ADDR. */
static inline void
insl (uint16_t port, void *addr, size_t cnt)
{
  asm volatile ("cld; repne; insl"
                : "=D" (addr), "=c" (cnt)
                : "d" (port), "0" (addr), "1" (cnt)
                : "memory", "cc");
}

/* Writes byte DATA to PORT. */
static inline void
outb (uint16_t port, uint8_t data)
{
  asm volatile ("outb %0,%w1" : : "a" (data), "d" (port));
}

/* Writes to PORT each byte of data in the CNT-byte buffer
   starting at ADDR. */
static inline void
outsb (uint16_t port, const void *addr, size_t cnt)
{
  asm volatile ("cld; repne; outsb"
                : "=S" (addr), "=c" (cnt)
                : "d" (port), "0" (addr), "1" (cnt)
                : "cc");
}

/* Writes the 16-bit DATA to PORT. */
static inline void
outw (uint16_t port, uint16_t data)
{
  asm volatile ("outw %0,%w1" : : "a" (data), "d" (port));
}

/* Writes to PORT each 16-bit unit (halfword) of data in the
   CNT-halfword buffer starting at ADDR. */
static inline void
outsw (uint16_t port, const void *addr, size_t cnt)
{
  asm volatile ("cld; repne; outsw"
                : "=S" (addr), "=c" (cnt)
                : "d" (port), "0" (addr), "1" (cnt)
                : "cc");
}

/* Writes the 32-bit DATA to PORT. */
static inline void
outl (uint16_t port, uint32_t data)
{
  asm volatile ("outl %0,%w1" : : "a" (data), "d" (port));
}

/* Writes to PORT each 32-bit unit (word) of data in the CNT-word
   buffer starting at ADDR. */
static inline void
outsl (uint16_t port, const void *addr, size_t cnt)
{
  asm volatile ("cld; repne; outsl"
                : "=S" (addr), "=c" (cnt)
                : "d" (port), "0" (addr), "1" (cnt)
                : "cc");
}

#endif /* io.h */
