#ifndef DEVICES_PCI_H
#define DEVICES_PCI_H

#include "devices/resource.h"
#include "lib/kernel/list.h"

#define PCI_BAD_DEVICE 0xffff

/* PCI-to-PCI bridge related numbers */
#define PCI_BRIDGE_BASE_CLASS 0x06
#define PCI_BRIDGE_SUB_CLASS 0x04
#define PCI_BRIDGE_REG_SBUS 0x19
#define PCI_BRIDGE_HEADER_TYPE 0x01

/* Locations of registers in the configuration space */
#define PCI_REG_CLASS_INTERFACE 0x09
#define PCI_REG_CLASS_SUB 0x0a
#define PCI_REG_CLASS_BASE 0x0b
#define PCI_REG_HEADER_TYPE 0x0e
#define PCI_INTERRUPT_MASK_PIN 0xff00
#define PCI_INTERRUPT_MASK_LINE 0xff
#define PCI_REGNUM_INTERRUPT 15
#define PCI_REGNUM_BASE_ADDRESS 4

/* Base address related numbers */
#define PCI_NUM_BARS 6
#define PCI_BAR_TYPE_MASK 0x1
#define PCI_BAR_TYPE_MEM 0x0
#define PCI_BAR_TYPE_IO 0x1
#define PCI_BAR_MASK_MEM 0xfffffff0
#define PCI_BAR_MASK_IO 0xfffffffc

struct pci_dev 
{
  struct list_elem elem;

  /* <Bus, Device, Function> logically identify a unique PCI device */
  uint8_t bus_id;
  uint8_t devfn;

  /* Vendor and Device ID */
  uint16_t ven_id;
  uint16_t dev_id;

  /* Class code */
  uint8_t base_class;
  uint8_t sub_class;
  uint8_t interface;

  /* Resource space */
  struct resource resources[PCI_NUM_BARS];

  /* Interrupt */
  uint8_t irq;
};

void pci_init (void);
void pci_dump (void);

#endif /* devices/pci.h */
