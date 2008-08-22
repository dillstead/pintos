#define PCI_TRANSLATION_ENABLE 1

#include "devices/pci.h"
#include "threads/malloc.h"
#include "threads/interrupt.h"
#include "threads/pte.h"
#include "threads/io.h"
#include "pci_lookup.h"
#include <round.h>
#include <list.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

extern uint32_t *base_page_dir;

#define PCI_REG_ADDR	0xcf8
#define PCI_REG_DATA	0xcfc
#define pci_config_offset(bus, dev, func, reg)	(0x80000000 | ((bus) << 16) | ((dev) << 11) | ((func) << 8) | (reg & (~3)))

#define PCI_MAX_DEV_PER_BUS	32
#define PCI_MAX_FUNC_PER_DEV	8

#define PCI_HEADER_SZ		256

#define PCI_CMD_IO		0x001	/* enable io response */
#define PCI_CMD_MEMORY		0x002	/* enable memory response */
#define PCI_CMD_MASTER		0x004	/* enable bus mastering */
#define PCI_CMD_SPECIAL		0x008	/* enable special cycles */
#define PCI_CMD_INVALIDATE	0x010	/* memory write + invalidate */
#define PCI_CMD_PALETTE		0x020	/* palette snooping */
#define PCI_CMD_PARITY		0x040	/* parity checking */
#define PCI_CMD_WAIT		0x080	/* address/data stepping */
#define PCI_CMD_SERR		0x100	/* serr */
#define PCI_CMD_FASTBACK	0x200	/* back-to-back writing */
#define PCI_CMD_INTX_DISABLE	0x400	/* emulation disable */

#define PCI_STATUS_CAPLIST	0x10	/* capability list */
#define PCI_STATUS_66MHZ	0x20	/* 66mhz pci 2.1 bus */
#define PCI_STATUS_UDF		0x40	/* user definable features */
#define PCI_STATUS_FASTBACK	0x80	/* fast back to back */
#define PCI_STATUS_PARITY	0x100	/* parity error detected */
#define PCI_STATUS_DEVSEL	0x600	/* devsel mask */
#define PCI_STATUS_DEVSEL_FAST	0
#define PCI_STATUS_DEVSEL_MED	0x200
#define PCI_STATUS_DEVSEL_SLOW	0x400
#define PCI_STATUS_SIG_ABORT	0x0800	/* set on target abort */
#define PCI_STATUS_REC_ABORT	0x1000	/* master ack of abort */
#define PCI_STATUS_REC_ABORT_M	0x2000	/* set on master abort */
#define PCI_STATUS_SERR		0x4000	/* system error */
#define PCI_STATUS_PARITY2	0x8000	/* set on parity err */

#define PCI_HEADER_NORMAL	0
#define PCI_HEADER_BRIDGE	1
#define PCI_HEADER_CARDBUS	2

#define PCI_HEADER_MASK		0x7f
#define PCI_HEADER_MULTIFUNC	0x80

#define pci_get_reg_offset(x)	(&(((pci_config_header*)(NULL))->x))

#define PCI_VENDOR_INVALID	0xffff

#define PCI_BASEADDR_IO		0x00000001
#define PCI_BASEADDR_TYPEMASK	0x00000006
#define PCI_BASEADDR_32BIT	0x00000000
#define PCI_BASEADDR_64BIT	0x00000004
#define PCI_BASEADDR_PREFETCH	0x00000008
#define PCI_BASEADDR_ADDR32	0xfffffff0
#define PCI_BASEADDR_ADDR64	0xfffffffffffffff0
#define PCI_BASEADDR_IOPORT	0xfffffffc

#pragma pack(1)

struct pci_config_header
{
  uint16_t pci_vendor_id;
  uint16_t pci_device_id;

  uint16_t pci_command;
  uint16_t pci_status;

  uint8_t pci_revision;

  /* pci class */
  uint8_t pci_interface;
  uint8_t pci_minor;
  uint8_t pci_major;

  uint8_t pci_cachelinesz;
  uint8_t pci_latency;
  uint8_t pci_header;		/* header type */
  uint8_t pci_bist;		/* self test */

  uint32_t pci_base_reg[6];
  uint32_t pci_cis;		/* cardbus cis pointer */
  uint16_t pci_sub_vendor_id;
  uint16_t pci_sub_id;
  uint32_t pci_rom_addr;
  uint8_t pci_capabilities;
  uint8_t pci_resv[7];
  uint8_t pci_int_line;
  uint8_t pci_int_pin;
  uint8_t pci_min_gnt;
  uint8_t pci_max_latency;
};

#pragma pack()

#define PCI_BASE_COUNT		6
struct pci_dev
{
  struct pci_config_header pch;
  uint8_t bus, dev, func;
  int base_reg_size[PCI_BASE_COUNT];

  pci_handler_func *irq_handler;
  void *irq_handler_aux;

  struct list io_ranges;
  struct list_elem peer;
  struct list_elem int_peer;
};

enum pci_io_type
{ PCI_IO_MEM, PCI_IO_PORT };

/* represents a PCI IO range */
struct pci_io
{
  struct pci_dev *dev;
  enum pci_io_type type;
  size_t size;	/* bytes in range */
  union
  {
    void *ptr;	/* virtual memory address */
    int port;	/* io port */
  } addr;
  struct list_elem peer; /* linkage */
};


static void pci_write_config (int bus, int dev, int func, int reg,
			      int size, uint32_t data);
static uint32_t pci_read_config (int bus, int dev, int func, int reg,
				 int size);
static void pci_read_all_config (int bus, int dev, int func,
				 struct pci_config_header *pch);
static int pci_scan_bus (int bus);
static int pci_probe (int bus, int dev, int func,
		      struct pci_config_header *ph);
static int pci_pci_bridge (struct pci_dev *pd);
static void pci_power_on (struct pci_dev *pd);
static void pci_setup_io (struct pci_dev *pd);
static void pci_interrupt (struct intr_frame *);
static void pci_print_dev_info (struct pci_dev *pd);
static void *pci_alloc_mem (void *phys_ptr, int pages);

static struct list devices;
static struct list int_devices;

/* number of pages that have been allocated to pci devices in the pci zone */
static int num_pci_pages;

void
pci_init (void)
{
  list_init (&devices);
  list_init (&int_devices);

  num_pci_pages = 0;

  pci_scan_bus (0);
  pci_print_stats ();
}

struct pci_dev *
pci_get_device (int vendor, int device, int func, int n)
{
  struct list_elem *e;
  int count;

  count = 0;
  e = list_begin (&devices);
  while (e != list_end (&devices))
    {
      struct pci_dev *pd;

      pd = list_entry (e, struct pci_dev, peer);
      if (pd->pch.pci_vendor_id == vendor && pd->pch.pci_device_id == device)
	if (pd->func == func)
	  {
	    if (count == n)
	      return pd;
	    count++;
	  }

      e = list_next (e);
    }

  return NULL;
}

struct pci_dev *
pci_get_dev_by_class (int major, int minor, int iface, int n)
{
  struct list_elem *e;
  int count;

  count = 0;
  e = list_begin (&devices);
  while (e != list_end (&devices))
    {
      struct pci_dev *pd;

      pd = list_entry (e, struct pci_dev, peer);
      if (pd->pch.pci_major == major && pd->pch.pci_minor == minor &&
	  pd->pch.pci_interface == iface)
	{
	  if (count == n)
	    return pd;
	  count++;
	}

      e = list_next (e);
    }

  return NULL;

}

struct pci_io *
pci_io_enum (struct pci_dev *pio, struct pci_io *last)
{
  struct list_elem *e;

  if (last != NULL)
    e = list_next (&last->peer);
  else
    e = list_begin (&pio->io_ranges);

  if (e == list_end (&pio->io_ranges))
    return NULL;

  return list_entry (e, struct pci_io, peer);
}

void
pci_register_irq (struct pci_dev *pd, pci_handler_func * f, void *aux)
{
  int int_vec;
  ASSERT (pd != NULL);
  ASSERT (pd->irq_handler == NULL);

  pd->irq_handler_aux = aux;
  pd->irq_handler = f;
  int_vec = pd->pch.pci_int_line + 0x20;

  list_push_back (&int_devices, &pd->int_peer);

  /* ensure that pci interrupt is hooked */
  if (!intr_is_registered (int_vec))
    intr_register_ext (int_vec, pci_interrupt, "PCI");
}

void
pci_unregister_irq (struct pci_dev *pd)
{
  ASSERT (pd != NULL);

  intr_disable ();
  list_remove (&pd->int_peer);
  intr_enable ();

  pd->irq_handler = NULL;
  pd->irq_handler_aux = NULL;
}

size_t
pci_io_size (struct pci_io *pio)
{
  ASSERT (pio != NULL);

  return pio->size;
}

void
pci_reg_write32 (struct pci_io *pio, int reg, uint32_t data)
{
  ASSERT (pio != NULL);
  ASSERT ((unsigned) reg < pio->size);

  if (pio->type == PCI_IO_MEM)
    {
      *((uint32_t *) (pio->addr.ptr + reg)) = data;
    }
  else if (pio->type == PCI_IO_PORT)
    {
      outl (pio->addr.port + reg, data);
    }
  else
    PANIC ("pci: Invalid IO type\n");
}

void
pci_reg_write16 (struct pci_io *pio, int reg, uint16_t data)
{
  ASSERT (pio != NULL);
  ASSERT ((unsigned) reg < pio->size);

  if (pio->type == PCI_IO_MEM)
    {
      *((uint16_t *) (pio->addr.ptr + reg)) = data;
    }
  else if (pio->type == PCI_IO_PORT)
    {
      outw (pio->addr.port + reg, data);
    }
  else
    PANIC ("pci: Invalid IO type\n");
}

void
pci_reg_write8 (struct pci_io *pio, int reg, uint8_t data)
{
  ASSERT (pio != NULL);
  ASSERT ((unsigned) reg < pio->size);

  if (pio->type == PCI_IO_MEM)
    {
      *((uint8_t *) (pio->addr.ptr + reg)) = data;
    }
  else if (pio->type == PCI_IO_PORT)
    {
      outb (pio->addr.port + reg, data);
    }
  else
    PANIC ("pci: Invalid IO type\n");
}

uint32_t
pci_reg_read32 (struct pci_io *pio, int reg)
{
  uint32_t ret;

  ASSERT (pio != NULL);
  ASSERT ((unsigned) reg < pio->size);

  if (pio->type == PCI_IO_MEM)
    {
      ret = *((uint32_t *) (pio->addr.ptr + reg));
    }
  else if (pio->type == PCI_IO_PORT)
    {
      ret = inl (pio->addr.port + reg);
    }
  else
    PANIC ("pci: Invalid IO type\n");

  return ret;
}

uint16_t
pci_reg_read16 (struct pci_io * pio, int reg)
{
  uint16_t ret;

  ASSERT (pio != NULL);
  ASSERT ((unsigned) reg < pio->size);

  ret = 0;
  if (pio->type == PCI_IO_MEM)
    {
      ret = *((uint16_t *) (pio->addr.ptr + reg));
    }
  else if (pio->type == PCI_IO_PORT)
    {
      ret = inw (pio->addr.port + reg);
    }
  else
    PANIC ("pci: Invalid IO type\n");

  return ret;

}

uint8_t
pci_reg_read8 (struct pci_io * pio, int reg)
{
  uint8_t ret;

  ASSERT (pio != NULL);
  ASSERT ((unsigned) reg < pio->size);

  if (pio->type == PCI_IO_MEM)
    {
      ret = *((uint8_t *) (pio->addr.ptr + reg));
    }
  else if (pio->type == PCI_IO_PORT)
    {
      ret = inb (pio->addr.port + reg);
    }
  else
    PANIC ("pci: Invalid IO type\n");

  return ret;
}

void
pci_read_in (struct pci_io *pio UNUSED, int off UNUSED, size_t size UNUSED,
	     void *buf UNUSED)
{
  PANIC ("pci_read_in: STUB");
}

void
pci_write_out (struct pci_io *pio UNUSED, int off UNUSED, size_t size UNUSED,
	       const void *buf UNUSED)
{
  PANIC ("pci_write_out: STUB");
}

static void
pci_write_config (int bus, int dev, int func, int reg,
		  int size, uint32_t data)
{
  int config_offset;

  config_offset = pci_config_offset (bus, dev, func, reg);

  outl (PCI_REG_ADDR, config_offset);

  switch (size)
    {
    case 1:
      outb (PCI_REG_DATA + (reg & 3), data);
      break;

    case 2:
      outw (PCI_REG_DATA + (reg & 3), data);
      break;

    case 4:
      outl (PCI_REG_DATA, data);
      break;
    }
}

static uint32_t
pci_read_config (int bus, int dev, int func, int reg, int size)
{
  uint32_t ret;
  int config_offset;

  config_offset = pci_config_offset (bus, dev, func, reg);

  outl (PCI_REG_ADDR, config_offset);

  switch (size)
    {
    case 1:
      ret = inb (PCI_REG_DATA);
      break;
    case 2:
      ret = inw (PCI_REG_DATA);
      break;
    case 4:
      ret = inl (PCI_REG_DATA);
      break;
    default:
      PANIC ("pci: Strange config read size\n");
    }

  return ret;
}

/* read entire configuration header into memory */
static void
pci_read_all_config (int bus, int dev, int func,
		     struct pci_config_header *pch)
{
  unsigned int i;
  for (i = 0; i < ((sizeof (struct pci_config_header) + 3) & ~3) / 4; i++)
    {
      ((uint32_t *) pch)[i] = pci_read_config (bus, dev, func, i * 4, 4);
    }
}


/** scan PCI bus for all devices */
static int
pci_scan_bus (int bus)
{
  int dev;
  int max_bus;

  max_bus = 0;

  for (dev = 0; dev < PCI_MAX_DEV_PER_BUS; dev++)
    {
      struct pci_config_header pch;
      int func_cnt, func;

      pci_read_all_config (bus, dev, 0, &pch);

      if (pch.pci_vendor_id == PCI_VENDOR_INVALID)
	continue;

      func_cnt = 8;
      if (!(pch.pci_header & PCI_HEADER_MULTIFUNC))
	{
	  func_cnt = 1;
	}

      for (func = 0; func < func_cnt; func++)
	{
	  int retbus;
	  retbus = pci_probe (bus, dev, func, &pch);
	  if (retbus > max_bus)
	    max_bus = retbus;
	}
    }

  return max_bus;
}

/* get all information for a PCI device given bus/dev/func 
   add pci device to device list if applicable 
   return a new bus number if new bus is found
 */
static int
pci_probe (int bus, int dev, int func, struct pci_config_header *ph)
{
  struct pci_dev *pd;

  if (func != 0)
    {
      pci_read_all_config (bus, dev, func, ph);
      if (ph->pci_vendor_id == PCI_VENDOR_INVALID)
	return bus;
    }

  pd = malloc (sizeof (struct pci_dev));
  memcpy (&pd->pch, ph, sizeof (struct pci_config_header));
  pd->irq_handler = NULL;
  pd->irq_handler_aux = NULL;
  pd->bus = bus;
  pd->dev = dev;
  pd->func = func;

  list_init (&pd->io_ranges);
  list_push_back (&devices, &pd->peer);


  if (ph->pci_major == PCI_MAJOR_BRIDGE)
    {
      if (ph->pci_minor == PCI_MINOR_PCI)
	return pci_pci_bridge (pd);
    }

  pci_setup_io (pd);
  pci_power_on (pd);

  return bus;
}

static void
pci_setup_io (struct pci_dev *pd)
{
  int i;
  for (i = 0; i < PCI_BASE_COUNT; i++)
    {
      uint32_t tmp;
      struct pci_io *pio;

      if (pd->pch.pci_base_reg[i] == 0)
	{
	  continue;
	}

      /* determine io granularity.. */
      pci_write_config (pd->bus, pd->dev, pd->func,
			offsetof (struct pci_config_header, pci_base_reg[i]),
			4, ~0);

      tmp =
	pci_read_config (pd->bus, pd->dev, pd->func,
			 offsetof (struct pci_config_header, pci_base_reg[i]),
			 4);

      /* configure BAR to the default */
      pci_write_config (pd->bus, pd->dev, pd->func,
			offsetof (struct pci_config_header, pci_base_reg[i]),
			4, pd->pch.pci_base_reg[i]);

      pio = malloc (sizeof (struct pci_io));
      pio->dev = pd;

      if (tmp & PCI_BASEADDR_IO)
	{
	  pio->type = PCI_IO_PORT;
	  pio->size = (uint16_t) ((~tmp + 1) & 0xffff) + 1;
	  pio->addr.port = pd->pch.pci_base_reg[i] & ~1;
	}
      else
	{
	  uint32_t ofs;

	  pio->type = PCI_IO_MEM;
	  pio->size = ROUND_UP ((~tmp + 1), PGSIZE);
	  ofs = (pd->pch.pci_base_reg[i] & 0xfffffff0 & PGMASK);
	  pio->addr.ptr = pci_alloc_mem ((void *) pd->pch.pci_base_reg[i],
					 pio->size / PGSIZE);
	  if (pio->addr.ptr == NULL)
	    {
	      printf ("PCI: %d pages for %d:%d.%d failed - may not work\n",
		      pio->size / PGSIZE, pd->bus, pd->dev, pd->func);
	      free (pio);
	      pio = NULL;
	    }
	  else
	    {
	      pio->addr.ptr = (void *) ((uintptr_t) pio->addr.ptr + ofs);
	    }
	}

      /* add IO struct to device, if valid */
      if (pio != NULL)
	list_push_back (&pd->io_ranges, &pio->peer);
    }

}

static void
pci_power_on (struct pci_dev *pd UNUSED)
{
  /* STUB */
}

static int
pci_pci_bridge (struct pci_dev *pd)
{
  int max_bus;
  uint16_t command;

  /* put bus into offline mode */
  command = pd->pch.pci_command;
  command &= ~0x3;
  pci_write_config (pd->bus, pd->dev, pd->func,
		    offsetof (struct pci_config_header, pci_command),
		    2, command);
  pd->pch.pci_command = command;

  /* set up primary bus */
  pci_write_config (pd->bus, pd->dev, pd->func, 0x18, 1, pd->bus);
  /* secondary bus */
  pci_write_config (pd->bus, pd->dev, pd->func, 0x19, 1, pd->bus + 1);

  /* disable subordinates */
  pci_write_config (pd->bus, pd->dev, pd->func, 0x1a, 1, 0xff);

  /* scan this new bus */
  max_bus = pci_scan_bus (pd->bus + 1);

  /* set subordinate to the actual last bus */
  pci_write_config (pd->bus, pd->dev, pd->func, 0x1a, 1, max_bus);

  /* put online */
  command |= 0x03;
  pci_write_config (pd->bus, pd->dev, pd->func,
		    offsetof (struct pci_config_header, pci_command),
		    2, command);
  pd->pch.pci_command = command;

  return max_bus;
}

/* alert all PCI devices waiting on interrupt line that IRQ fired */
static void
pci_interrupt (struct intr_frame *frame)
{
  struct list_elem *e;
  int int_line;

  int_line = frame->vec_no - 0x20;
  e = list_begin (&int_devices);
  while (e != list_end (&int_devices))
    {
      struct pci_dev *pd;

      pd = list_entry (e, struct pci_dev, int_peer);
      if (pd->pch.pci_int_line == int_line)
	pd->irq_handler (pd->irq_handler_aux);
      e = list_next (e);
    }
}

/* display information on all USB devices */
void
pci_print_stats (void)
{
  struct list_elem *e;

  e = list_begin (&devices);
  while (e != list_end (&devices))
    {
      struct pci_dev *pd;

      pd = list_entry (e, struct pci_dev, peer);
      pci_print_dev_info (pd);

      e = list_next (e);
    }
}

static void
pci_print_dev_info (struct pci_dev *pd)
{
  printf ("PCI Device %d:%d.%d (%x,%x): %s - %s (%s) IRQ %d\n",
	  pd->bus, pd->dev, pd->func,
	  pd->pch.pci_vendor_id,
	  pd->pch.pci_device_id,
	  pci_lookup_vendor (pd->pch.pci_vendor_id),
	  pci_lookup_device (pd->pch.pci_vendor_id, pd->pch.pci_device_id),
	  pci_lookup_class (pd->pch.pci_major, pd->pch.pci_minor,
			    pd->pch.pci_interface), pd->pch.pci_int_line);
}

void
pci_mask_irq (struct pci_dev *pd)
{
  intr_irq_mask (pd->pch.pci_int_line);
}

void
pci_unmask_irq (struct pci_dev *pd)
{
  intr_irq_unmask (pd->pch.pci_int_line);
}

void
pci_write_config16 (struct pci_dev *pd, int off, uint16_t data)
{
  pci_write_config (pd->bus, pd->dev, pd->func, off, 2, data);
}

void
pci_write_config32 (struct pci_dev *pd, int off, uint32_t data)
{
  pci_write_config (pd->bus, pd->dev, pd->func, off, 4, data);
}

void
pci_write_config8 (struct pci_dev *pd, int off, uint8_t data)
{
  pci_write_config (pd->bus, pd->dev, pd->func, off, 1, data);
}

uint8_t
pci_read_config8 (struct pci_dev *pd, int off)
{
  return pci_read_config (pd->bus, pd->dev, pd->func, off, 1);
}

uint16_t
pci_read_config16 (struct pci_dev * pd, int off)
{
  return pci_read_config (pd->bus, pd->dev, pd->func, off, 2);
}

uint32_t
pci_read_config32 (struct pci_dev * pd, int off)
{
  return pci_read_config (pd->bus, pd->dev, pd->func, off, 4);
}


/** allocate PCI memory pages for PCI devices */
static void *
pci_alloc_mem (void *phys_ptr, int pages)
{
  void *vaddr;
  int i;

  phys_ptr = (void *) ((uintptr_t) phys_ptr & ~PGMASK);

  /* not enough space to allocate? */
  if ((unsigned) (num_pci_pages + pages) >= (unsigned) PCI_ADDR_ZONE_PAGES)
    {
      return NULL;
    }

  /* insert into PCI_ZONE */
  for (i = 0; i < pages; i++)
    {
      uint32_t pte_idx = (num_pci_pages + i) % 1024;
      uint32_t pde_idx = (num_pci_pages + i) / 1024;
      uint32_t *pt;
      uint32_t pte;

      pde_idx += pd_no ((void *) PCI_ADDR_ZONE_BEGIN);
      pte = ((uint32_t) phys_ptr + (i * PGSIZE)) | PTE_P | PTE_W | PTE_CD;
      pt = (uint32_t *) (ptov (base_page_dir[pde_idx] & ~PGMASK));
      pt[pte_idx] = pte;
    }

  vaddr = (void *) (PCI_ADDR_ZONE_BEGIN + (num_pci_pages * PGSIZE));
  num_pci_pages += pages;

  return vaddr;
}
