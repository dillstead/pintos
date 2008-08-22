/**
 *
 * Actual EHCI will be implemented later
 *
 * For now, we just deactivate the EHCI controller and routing circuitry
 * so that any USB2.0 devices activated by the BIOS will show up on the 
 * USB1.1 controllers instead of being routed to EHCI and therefore "invisible"
 * to the system.
 *
 */

#include "devices/pci.h"
#include "devices/usb.h"
#include <stdlib.h>
#include <stdio.h>

/* capability registers */
#define EHCI_REG_CAPLENGTH	0x00

/* operational regisers - must be offset by op_base */
#define EHCI_REG_CONFIGFLAG	0x40

void ehci_init (void);


void
ehci_init (void)
{
  struct pci_dev *pd;
  int dev_num;

  dev_num = 0;
  while ((pd = pci_get_dev_by_class (PCI_MAJOR_SERIALBUS, PCI_MINOR_USB,
				     PCI_USB_IFACE_EHCI, dev_num)) != NULL)
    {
      struct pci_io *io;
      uint8_t op_base;

      dev_num++;
      io = pci_io_enum (pd, NULL);
      if (io == NULL)
	{
	  printf ("IO not found on EHCI device?\n");
	  continue;
	}
      printf ("Disabling the EHCI controller #%d\n", dev_num - 1);
      op_base = pci_reg_read8 (io, EHCI_REG_CAPLENGTH);

      /* turn off EHCI routing */
      pci_reg_write32 (io, EHCI_REG_CONFIGFLAG + op_base, 0);
    }
}
