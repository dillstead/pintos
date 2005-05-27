#include <list.h>
#include <stdio.h>
#include "devices/pci.h"
#include "devices/usb.h"

extern struct list pci_dev_list;

void usb_init(void)
{
  printf ("\n");
  printf ("USB: Initializing\n");

  /* Scan PCI devices for USB controllers */
  struct list_elem *e;

  for (e = list_begin (&pci_dev_list); e != list_end (&pci_dev_list);
       e = list_next (e))
    {
      struct pci_dev *dev = list_entry (e, struct pci_dev, elem);

      if (dev->base_class == USB_BASE_CLASS &&
	  dev->sub_class == USB_SUB_CLASS)
	{
	  printf ("USB: Found controller at %04x:%02x:%02x.%x\n",
		  0, dev->bus_id, dev->devfn >> 4, dev->devfn & 0xf);
	}
    }
  printf ("USB: Initialization done\n");
  printf ("\n");
}
