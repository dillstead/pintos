#ifndef DEVICES_PCI_H
#define DEVICES_PCI_H

#include <stdint.h>
#include <stddef.h>

/**************************/
/* some major/minor pairs */
/**************************/
#define PCI_MAJOR_EARLY			0
#define PCI_MINOR_VGA			1

#define PCI_MAJOR_MASS_STORAGE		1	/* mass storage controller */
#define PCI_MINOR_SCSI			0
#define PCI_MINOR_IDE			1
#define PCI_MINOR_FLOPPY		2
#define PCI_MINOR_IPI			3
#define PCI_MINOR_RAID			4
#define PCI_MINOR_MS_OTHER		0x80

#define PCI_MAJOR_NETWORK		2	/* network controller */
#define PCI_MINOR_ETHERNET		0
#define PCI_MINOR_TOKENRING		1
#define PCI_MINOR_FDDI			2
#define PCI_MINOR_ATM			3
#define PCI_MINOR_ISDN			4
#define PCI_MINOR_NET_OTHER		0x80

#define PCI_MAJOR_DISPLAY		3	/* display controller */
#define PCI_MAJOR_MULTIMEDIA		4	/* multimedia device */
#define PCI_MAJOR_MEMORY		5	/* memory controller */

#define PCI_MAJOR_BRIDGE		6	/* bus bridge controller */
#define PCI_MINOR_HOST			0
#define PCI_MINOR_ISA			1
#define PCI_MINOR_EISA			2
#define PCI_MINOR_MCA			3
#define PCI_MINOR_PCI			4
#define PCI_MINOR_PCMCIA		5
#define PCI_MINOR_NUBUS			6
#define PCI_MINOR_CARDBUS		7
#define PCI_MINOR_RACEWAY		8

#define PCI_MAJOR_SIMPLE_COMM		7

#define PCI_MAJOR_BASE_PERIPHERAL	8
#define PCI_MINOR_PIC			0
#define PCI_MINOR_DMA			1
#define PCI_MINOR_TIMER			2
#define PCI_MINOR_RTC			3
#define PCI_MINOR_HOTPLUG		4


#define	PCI_MAJOR_INPUT			9
#define PCI_MAJOR_DOCK			10
#define PCI_MAJOR_PROCESSOR		11
#define PCI_MINOR_386			0
#define PCI_MINOR_486			1
#define PCI_MINOR_PENTIUM		2
#define PCI_MINOR_ALPHA			0x10
#define PCI_MINOR_PPC			0x20
#define PCI_MINOR_MIPS			0x30
#define PCI_MINOR_COPROC		0x40

#define PCI_MAJOR_SERIALBUS		12
#define PCI_MINOR_FIREWIRE		0
#define PCI_MINOR_ACCESS		1
#define PCI_MINOR_SSA			2
#define PCI_MINOR_USB			3
#define PCI_USB_IFACE_UHCI		0
#define PCI_USB_IFACE_OHCI		0x10
#define PCI_USB_IFACE_EHCI		0x20
#define PCI_MINOR_FIBRE			4
#define PCI_MAJOR_UNDEF			0xff

typedef void pci_handler_func (void *AUX);

/* used for io range for a pci device */
struct pci_io;

/* structure representing a specific pci device/function */
struct pci_dev;

void pci_init (void);
struct pci_dev *pci_get_device (int vendor, int device, int func, int n);
struct pci_dev *pci_get_dev_by_class (int major, int minor, int iface, int n);
struct pci_io *pci_io_enum (struct pci_dev *, struct pci_io *last);
void pci_register_irq (struct pci_dev *, pci_handler_func *, void *AUX);
void pci_unregister_irq (struct pci_dev *);
size_t pci_io_size (struct pci_io *);

void pci_write_config8 (struct pci_dev *, int reg, uint8_t);
void pci_write_config16 (struct pci_dev *, int reg, uint16_t);
void pci_write_config32 (struct pci_dev *, int reg, uint32_t);
uint8_t pci_read_config8 (struct pci_dev *, int reg);
uint16_t pci_read_config16 (struct pci_dev *, int reg);
uint32_t pci_read_config32 (struct pci_dev *, int reg);

void pci_reg_write32 (struct pci_io *, int reg, uint32_t);
void pci_reg_write16 (struct pci_io *, int reg, uint16_t);
void pci_reg_write8 (struct pci_io *, int reg, uint8_t);
uint32_t pci_reg_read32 (struct pci_io *, int reg);
uint16_t pci_reg_read16 (struct pci_io *, int reg);
uint8_t pci_reg_read8 (struct pci_io *, int reg);

void pci_read_in (struct pci_io *, int off, size_t sz, void *buf);
void pci_write_out (struct pci_io *, int off, size_t sz, const void *buf);
void pci_print_stats (void);
void pci_mask_irq (struct pci_dev *);
void pci_unmask_irq (struct pci_dev *);

#endif
