#ifndef HEADER_IO_H
#define HEADER_IO_H 1

#include <stddef.h>
#include <stdint.h>

static inline uint8_t
inb (uint16_t port)
{
  uint8_t data;
  asm volatile ("inb %w1,%0" : "=a" (data) : "d" (port));
  return data;
}

static inline void
insb (uint16_t port, void *addr, size_t cnt)
{
  asm volatile ("cld; repne; insb"
                : "=D" (addr), "=c" (cnt)
                : "d" (port), "0" (addr), "1" (cnt)
                : "memory", "cc");
}

static inline uint16_t
inw (uint16_t port)
{
  uint16_t data;
  asm volatile ("inw %w1,%0" : "=a" (data) : "d" (port));
  return data;
}

static inline void
insw (uint16_t port, void *addr, size_t cnt)
{
  asm volatile ("cld; repne; insw"
                : "=D" (addr), "=c" (cnt)
                : "d" (port), "0" (addr), "1" (cnt)
                : "memory", "cc");
}

static inline uint32_t
inl (uint16_t port)
{
  uint32_t data;
  asm volatile ("inl %w1,%0" : "=a" (data) : "d" (port));
  return data;
}

static inline void
insl(uint16_t port, void *addr, size_t cnt)
{
  asm volatile ("cld; repne; insl"
                : "=D" (addr), "=c" (cnt)
                : "d" (port), "0" (addr), "1" (cnt)
                : "memory", "cc");
}

static inline void
outb(uint16_t port, uint8_t data)
{
  asm volatile ("outb %0,%w1" : : "a" (data), "d" (port));
}

static inline void
outsb (uint16_t port, const void *addr, size_t cnt)
{
  asm volatile ("cld; repne; outsb"
                : "=S" (addr), "=c" (cnt)
                : "d" (port), "0" (addr), "1" (cnt)
                : "cc");
}

static inline void
outw (uint16_t port, uint16_t data)
{
  asm volatile ("outw %0,%w1" : : "a" (data), "d" (port));
}

static inline void
outsw (uint16_t port, const void *addr, size_t cnt)
{
  asm volatile ("cld; repne; outsw"
                : "=S" (addr), "=c" (cnt)
                : "d" (port), "0" (addr), "1" (cnt)
                : "cc");
}

static inline void
outl (uint16_t port, uint32_t data)
{
  asm volatile ("outl %0,%w1" : : "a" (data), "d" (port));
}

#endif /* io.h */
