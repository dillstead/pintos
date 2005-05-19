#include <debug.h>
#include <stdbool.h>
#include <stdio.h>
#include "threads/io.h"
#include "devices/pci.h"

static uint32_t
pci_read_config (unsigned bus, unsigned dev, unsigned func, unsigned reg) 
{
  ASSERT (bus < 256);
  ASSERT (dev < 32);
  ASSERT (func < 8);
  ASSERT (reg < 64);

  outl (0xcf8,
        0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (reg << 2));
  return inl (0xcfc);
}

static bool
scan_device (int bus, int dev, int func) 
{
  uint32_t cfg[16];
  int dev_id, vendor_id;
  int line;
  int reg;

  for (reg = 0; reg < 16; reg++)
    cfg[reg] = pci_read_config (bus, dev, func, reg);
  dev_id = cfg[0] >> 16;
  vendor_id = cfg[0] & 0xffff;

  if (dev_id == 0 || dev_id == 0xffff
      || vendor_id == 0 || vendor_id == 0xffff)
    return 0;

  printf ("%04x:%02x:%02x.%x PCI device %04x:%04x\n",
          0, bus, dev, func, vendor_id, dev_id);
  for (line = 0; line < 4; line++) 
    {
      int byte;

      printf ("%02x:", line * 16);
      for (byte = 0; byte < 16; byte++)
        printf (" %02x", ((uint8_t *) cfg)[line * 16 + byte]);
      printf ("\n");
    }
  printf ("\n");

  return cfg[3] & 0x00800000;
}

void
pci_scan (void) 
{
  int dev;

  printf ("PCI BUS: Scanning \n");
  
  for (dev = 0; dev < 32; dev++) 
    {
      if (scan_device (0, dev, 0)) 
        {
          int func;

          for (func = 1; func < 8; func++)
            scan_device (0, dev, func);
        }
    }

  printf ("PCI BUS: Done \n");
}
