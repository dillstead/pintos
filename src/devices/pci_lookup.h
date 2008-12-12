#ifndef PCILOOKUP_H
#define PCILOOKUP_H

#ifdef PCI_TRANSLATION_ENABLE

struct pci_vendor
{
  uint16_t vendor_id;
  char *vendor;
};

struct pci_vendor pci_vendor_table[] = {
  {0x1013, "Cirrus Logic"},
  {0x10de, "nVidia"},
  {0x10EC, "Realtek"},
  {0x11c1, "Agere Systems"},
  {0x8086, "Intel"}
};

struct pci_device
{
  uint16_t vendor_id;
  uint16_t device_id;
  char *device;
};

struct pci_device pci_device_table[] = {
  {0x1013, 0x00b8, "CL-GD5446"},
  {0x10ec, 0x8029, "RTL8029 - NE2K Compatible"},
  {0x10ec, 0x8139, "RTL8139"},
  {0x8086, 0x1237, "82441FX"},
  {0x8086, 0x7000, "82371SB_ISA"},
  {0x8086, 0x7010, "82371SB_IDE"},
  {0x8086, 0x7020, "82371SB_USB"},
  {0x8086, 0x7113, "82371AB/EB/MB_ACPI"}
};


#define PCI_DEVICE_TABLE_SZ	(sizeof (pci_device_table) / sizeof (struct pci_device ))
#define PCI_VENDOR_TABLE_SZ	(sizeof (pci_vendor_table) / sizeof (struct pci_vendor))

/** Too lazy to to a binary search... */

const char *pci_lookup_vendor (uint16_t vendor);
const char *pci_lookup_device (uint16_t vendor, uint16_t device);

const char *
pci_lookup_vendor (uint16_t vendor)
{
  unsigned int i;
  for (i = 0; i < PCI_VENDOR_TABLE_SZ; i++)
    {
      if (pci_vendor_table[i].vendor_id > vendor)
	break;
      if (pci_vendor_table[i].vendor_id == vendor)
	return pci_vendor_table[i].vendor;
    }

  return "Unknown Vendor";
}

const char *
pci_lookup_device (uint16_t vendor, uint16_t device)
{
  unsigned int i;
  for (i = 0; i < PCI_DEVICE_TABLE_SZ; i++)
    {
      if (pci_device_table[i].vendor_id > vendor)
	break;
      if (pci_device_table[i].vendor_id == vendor &&
	  pci_device_table[i].device_id == device)
	return pci_device_table[i].device;
    }

  return "Unknown Device";
}

#else

#define pci_lookup_vendor(x)	"Unknown Vendor"
#define pci_lookup_device(x,y)	"Unknown Device"

#endif

struct pci_class
{
  uint8_t major;
  uint8_t minor;
  uint8_t iface;
  char *name;
};

struct pci_class pci_class_table[] = {
  {0, 0, 0, "Pre-PCI 2.0 Non-VGA Device"},
  {0, 1, 0, "Pre-PCI 2.0 VGA Device"},
  {1, 0, 0, "SCSI Controller"},
  {1, 1, 0, "IDE Controller"},
  {1, 2, 0, "Floppy Disk Controller"},
  {2, 0, 0, "Ethernet"},
  {3, 0, 0, "VGA Controller"},
  {3, 1, 0, "XGA Controller"},
  {5, 0, 0, "Memory Controller - RAM"},
  {5, 1, 0, "Memory Controller - Flash"},
  {6, 0, 0, "PCI Host"},
  {6, 1, 0, "PCI-ISA Bridge"},
  {6, 2, 0, "PCI-EISA Bridge"},
  {6, 4, 0, "PCI-PCI Bridge"},
  {6, 5, 0, "PCI-PCMCIA Bridge"},
  {12, 0, 0, "Firewire Adapter"},
  {12, 3, 0, "USB 1.1 Controller (UHCI)"},
  {12, 3, 0x10, "USB 1.1 Controller (OHCI)"},
  {12, 3, 0x20, "USB 2.0 Controller (EHCI)"}
};

#define PCI_CLASS_TABLE_SZ (sizeof(pci_class_table) / sizeof(struct pci_class))

const char *pci_lookup_class (uint8_t major, uint8_t minor, uint8_t iface);
const char *
pci_lookup_class (uint8_t major, uint8_t minor, uint8_t iface)
{
  unsigned int i;
  for (i = 0; i < PCI_CLASS_TABLE_SZ; i++)
    {
      if (pci_class_table[i].major > major)
	break;
      if (pci_class_table[i].major != major)
	continue;
      if (pci_class_table[i].minor != minor)
	continue;
      if (pci_class_table[i].iface != iface)
	continue;
      return pci_class_table[i].name;
    }

  return "Unknown Type";
}

#endif
