#include <debug.h>
#include <list.h>
#include <stdbool.h>
#include <stdio.h>
#include "devices/pci.h"
#include "threads/io.h"
#include "threads/malloc.h"

struct list pci_dev_list;

static void scan_bus(uint8_t bus);

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

static void
pci_dump_dev (struct pci_dev *dev) 
{
  printf ("%04x:%02x:%02x.%x PCI device %04x:%04x\n", 
	  0, dev->bus_id, dev->devfn >> 4, dev->devfn & 0xf, 
	  dev->ven_id, dev->dev_id);
  printf ("Class: %02x:%02x:%02x\n", 
	  dev->base_class, dev->sub_class, dev->interface);
}

static bool
scan_device (uint8_t bus, uint8_t dev, uint8_t func) 
{
  uint32_t cfg[16];
  uint8_t *byte_cfg;
  int dev_id, vendor_id;
  int line;
  int reg;
  struct pci_dev *new_pci_dev;

  byte_cfg = (uint8_t *) cfg;

  /* Read configuration space header */
  for (reg = 0; reg < 16; reg++)
    cfg[reg] = pci_read_config (bus, dev, func, reg);
  
  /* Get vendor and device ID */
  dev_id = cfg[0] >> 16;
  vendor_id = cfg[0] & 0xffff;

  if (dev_id == 0 || dev_id == PCI_BAD_DEVICE || 
      vendor_id == 0 || vendor_id == PCI_BAD_DEVICE)
    return 0;

  /* We have a valid PCI device, set it up */
  new_pci_dev = malloc (sizeof *new_pci_dev);
  if (!new_pci_dev)
    PANIC ("couldn't allocate memory for PCI device");

  new_pci_dev->bus_id = bus;
  new_pci_dev->devfn = (dev << 4) | func;
  new_pci_dev->ven_id = vendor_id;
  new_pci_dev->dev_id = dev_id;
  new_pci_dev->base_class = byte_cfg[PCI_REG_CLASS_BASE];
  new_pci_dev->sub_class = byte_cfg[PCI_REG_CLASS_SUB];
  new_pci_dev->interface = byte_cfg[PCI_REG_CLASS_INTERFACE];
  list_push_front (&pci_dev_list, &new_pci_dev->elem);
  
  /* Debugging output */
  pci_dump_dev(new_pci_dev);
  for (line = 0; line < 16; line++) 
    {
      int byte;
      
      printf ("%02x:", line * 4);
      for (byte = 3; byte >= 0; byte--)
	printf (" %02x", byte_cfg[line * 4 + byte]);
      printf ("\n");
    }
  printf ("\n");
  
  /* If device is PCI-to-PCI bridge, scan the bus behind it */
  if (new_pci_dev->base_class == PCI_BRIDGE_BASE_CLASS &&
      new_pci_dev->sub_class == PCI_BRIDGE_SUB_CLASS &&
      (byte_cfg[PCI_REG_HEADER_TYPE] & 0x3f) == PCI_BRIDGE_HEADER_TYPE)
    scan_bus(byte_cfg[PCI_BRIDGE_REG_SBUS]);
  
  return byte_cfg[PCI_REG_HEADER_TYPE] & 0x80;
}

static void
scan_bus(uint8_t bus)
{
  uint8_t dev;

  printf ("PCI: Scanning bus (%u)\n", bus);

  for (dev = 0; dev < 32; dev++) 
    {
      /* Returns true if device is multi-function */
      if (scan_device (bus, dev, 0)) 
        {
          int func;

          for (func = 1; func < 8; func++)
            scan_device (bus, dev, func);
        }
    }
  printf ("PCI: Done (%u)\n", bus);
}

void
pci_init (void) 
{
  printf ("PCI: Initializating\n");
  list_init (&pci_dev_list);
  scan_bus(0);  
  printf ("PCI: Initialization done\n");
}

void
pci_dump (void)
{
  struct list_elem *e;

  for (e = list_begin (&pci_dev_list); e != list_end (&pci_dev_list);
       e = list_next (e))
    {
      struct pci_dev *dev = list_entry (e, struct pci_dev, elem);
      pci_dump_dev(dev);
    }
}
