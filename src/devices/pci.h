#include "lib/kernel/list.h"

struct pci_dev {
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
};

#define PCI_BAD_DEVICE 0xffff

/* PCI-to-PCI bridge related numbers */
#define PCI_BRIDGE_BASE_CLASS 0x06
#define PCI_BRIDGE_SUB_CLASS 0x04
#define PCI_BRIDGE_REG_SBUS 0x19
#define PCI_BRIDGE_HEADER_TYPE 0x01

/* Locations of registers in the typical configuration space */
#define PCI_REG_CLASS_INTERFACE 0x09
#define PCI_REG_CLASS_SUB 0x0a
#define PCI_REG_CLASS_BASE 0x0b
#define PCI_REG_HEADER_TYPE 0x0e

void pci_init (void);
void pci_dump (void);
