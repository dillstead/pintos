#include "devices/usb.h"
#include "devices/usb_hub.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "devices/timer.h"
#include <kernel/bitmap.h>
#include <string.h>
#include <stdio.h>

#define ADDRS_PER_HOST		127	/* number of configurable addresses */
#define ADDR_DEFAULT		0	/* default configuration address */
#define ADDR_FIRST		1	/* first configurable address */

/** sometimes used for ''value' */
/* used in desc_header */
#define SETUP_DESC_DEVICE	1
#define SETUP_DESC_CONFIG	2
#define SETUP_DESC_STRING	3
#define SETUP_DESC_IFACE	4
#define SETUP_DESC_ENDPOINT	5
#define SETUP_DESC_DEVQUAL	6
#define SETUP_DESC_SPDCONFIG	7
#define SETUP_DESC_IFACEPWR	8

#pragma pack(1)
struct desc_header
{
  uint8_t length;
  uint8_t type;
};

/**
 * More details can be found in chapter 9 of the usb1.1 spec
 */

struct device_descriptor
{
  struct desc_header hdr;
  uint16_t usb_spec;
  uint8_t dev_class;
  uint8_t dev_subclass;
  uint8_t dev_proto;
  uint8_t max_pktsz;

  uint16_t vendor_id;
  uint16_t product_id;
  uint16_t device_id;
  uint8_t manufacturer;
  uint8_t product;
  uint8_t serial;
  uint8_t num_configs;
};

struct device_qualifier
{
  uint16_t usb_spec;
  uint8_t dev_class;
  uint8_t dev_subclass;
  uint8_t dev_proto;
  uint8_t max_pktsz;
  uint8_t num_configs;
  uint8_t resv;
};

struct config_descriptor
{
  struct desc_header hdr;
  uint16_t total_length;
  uint8_t num_ifaces;
  uint8_t config_val;
  uint8_t config_desc;
  uint8_t attributes;
  uint8_t max_power;
};

struct interface_descriptor
{
  struct desc_header hdr;
  uint8_t iface_num;
  uint8_t alt_setting;
  uint8_t num_endpoints;
  uint8_t iface_class;
  uint8_t iface_subclass;
  uint8_t iface_proto;
  uint8_t iface_desc;
};

struct endpoint_descriptor
{
  struct desc_header hdr;
  /* end point address */
  uint8_t endpoint_num:4;
  uint8_t resv1:3;
  uint8_t direction:1;

  /* attributes */
  uint8_t transfer:2;
  uint8_t synch:2;
  uint8_t usage:2;
  uint8_t resv2:2;

  uint16_t max_pktsz;
  uint8_t interval;

};

#pragma pack()

#define USB_CLASS_HUB		0x09
struct host
{
  struct usb_host *dev;		/* host driver */
  host_info info;		/* private to host */
  struct bitmap *usb_dev_addrs;	/* addrs used on device */
  struct list host_usb_devs;	/* usb devices on host */
  struct list_elem peers;	/* other hosts on system */
};

struct class
{
  struct usb_class *dev;	/* class driver */
  struct list usb_ifaces;	/* usb devices on class */
  struct list_elem peers;
};

static struct list usb_dev_list;	/* list of all usb devices */
static struct list class_list;	/* list of all classes */
static struct list host_list;	/* list of all hosts on system */
static struct lock usb_sys_lock;	/* big usb lock */

#define usb_lock()		lock_acquire(&usb_sys_lock)
#define usb_unlock()		lock_release(&usb_sys_lock)

static void usb_scan_devices (struct host *h);
static struct usb_dev *usb_configure_default (struct host *h);
static char *usb_get_string (struct usb_dev *d, int ndx);
static struct class *usb_get_class_by_id (int id);
static void usb_setup_dev_addr (struct usb_dev *dev);
static void usb_config_dev (struct usb_dev *dev, int config_val);
static int usb_load_config (struct usb_dev *dev, int idx, void *data,
			    int dsz);
static int usb_tx_all (struct usb_endpoint *eop, void *buf,
		       int max_bytes, int bailout, bool in);
static void usb_attach_interfaces (struct usb_dev *dev);
static void usb_apply_class_to_interfaces (struct class *c);
static size_t wchar_to_ascii (char *dst, const char *src);

extern void uhci_init (void);
extern void ehci_init (void);

void
usb_init (void)
{
  list_init (&host_list);
  list_init (&class_list);
  list_init (&usb_dev_list);
  lock_init (&usb_sys_lock);

  usb_hub_init ();

  /* add usb hosts */
  printf ("Initializing EHCI\n");
  ehci_init ();
  printf ("Initializing UHCI\n");
  uhci_init ();
}

/* add host controller device to usb layer */
void
usb_register_host (struct usb_host *uh, host_info info)
{
  struct host *h;

  h = malloc (sizeof (struct host));
  h->dev = uh;
  h->info = info;
  h->usb_dev_addrs = bitmap_create (ADDRS_PER_HOST);
  list_init (&h->host_usb_devs);

  usb_lock ();
  list_push_back (&host_list, &h->peers);
  usb_unlock ();

  usb_scan_devices (h);
}

int
usb_register_class (struct usb_class *uc)
{
  struct class *c;

  usb_lock ();

  /* check to make sure class is not in list */
  if (usb_get_class_by_id (uc->class_id) != NULL)
    {
      usb_unlock ();
      return -1;
    }


  /* add class to class list */
  c = malloc (sizeof (struct class));
  c->dev = uc;
  list_init (&c->usb_ifaces);
  list_push_back (&class_list, &c->peers);

  usb_apply_class_to_interfaces (c);

  usb_unlock ();

  return 0;
}

static void
usb_scan_devices (struct host *h)
{
  /* scan until there all devices using default pipe are configured */

  printf ("USB: scanning devices...\n");
  while (1)
    {
      struct usb_dev *dev;

      dev = usb_configure_default (h);
      if (dev == NULL)
	break;

      if (dev->ignore_device == false)
	printf ("USB Device %d: %s (%s)\n",
		dev->addr, dev->product, dev->manufacturer);

      list_push_back (&h->host_usb_devs, &dev->host_peers);
      list_push_back (&usb_dev_list, &dev->sys_peers);
      usb_attach_interfaces (dev);
    }
}

static int
send_status (struct host *h, host_eop_info cfg_eop) 
{
  int sz;
  int err;

  h->dev->set_toggle (cfg_eop, 1);
  sz = 0;
  err = h->dev->tx_pkt (cfg_eop, USB_TOKEN_OUT, NULL, 0, 0, &sz, true);
  if (err)
    printf ("error %d in status transaction\n", err);
  return err;
}

static struct usb_dev *
usb_configure_default (struct host *h)
{
  struct usb_dev *dev;
  struct usb_setup_pkt sp;
  char data[256];
  struct device_descriptor *dd;
  host_dev_info hi;
  host_eop_info cfg_eop;
  bool ignore_device = false;
  int err, sz; 
  unsigned txed;
  int config_val;

  hi = h->dev->create_dev_channel (h->info, ADDR_DEFAULT, USB_VERSION_1_1);
  cfg_eop = h->dev->create_eop (hi, 0, 64);

  /* Get first 8 bytes of device descriptor. */
  sp.recipient = USB_SETUP_RECIP_DEV;
  sp.type = USB_SETUP_TYPE_STD;
  sp.direction = 1;
  sp.request = REQ_STD_GET_DESC;
  sp.value = SETUP_DESC_DEVICE << 8;
  sp.index = 0;
  sp.length = 8;

  err =
    h->dev->tx_pkt (cfg_eop, USB_TOKEN_SETUP, &sp, 0, sizeof (sp), NULL,
		    true);
  if (err != USB_HOST_ERR_NONE)
    goto error;

  dd = (void *) &data;
  memset (dd, 0, sizeof (data));
  txed = 0;
  err = h->dev->tx_pkt (cfg_eop, USB_TOKEN_IN, data, 0, 8, &sz, true);
  if (err != USB_HOST_ERR_NONE)
    goto error;
  
  err = send_status (h, cfg_eop);
  if (err != USB_HOST_ERR_NONE)
    return NULL;

  if (dd->usb_spec == USB_VERSION_1_0)
    {
      /* USB 1.0 devices have strict schedule requirements not yet supported */
      printf ("USB 1.0 device detected - skipping\n");
      goto error;
    }

  /* Now we know the max packet size. */
  if (dd->max_pktsz != 8 && dd->max_pktsz != 16
      && dd->max_pktsz != 32 && dd->max_pktsz != 64)
    goto error;

  h->dev->remove_eop (cfg_eop);
  cfg_eop = h->dev->create_eop (hi, 0, dd->max_pktsz);

  /* Get the whole descriptor. */
  sp.length = sizeof *dd;
  err =
    h->dev->tx_pkt (cfg_eop, USB_TOKEN_SETUP, &sp, 0, sizeof (sp), NULL,
		    true);
  if (err != USB_HOST_ERR_NONE)
    goto error;

  txed = 0;
  while (txed < sizeof *dd)
    {
      err = h->dev->tx_pkt (cfg_eop, USB_TOKEN_IN, data + txed, 0,
                            sizeof *dd - txed, &sz, true);
      if (err)
        goto error;
      txed += sz;
    }
  
  err = send_status (h, cfg_eop);
  if (err != USB_HOST_ERR_NONE)
    return NULL;

  if (dd->num_configs == 0)
    ignore_device = true;

  /* device exists - create device structure */
  dev = malloc (sizeof (struct usb_dev));
  memset (dev, 0, sizeof (struct usb_dev));
  list_init (&dev->interfaces);

  dev->ignore_device = ignore_device;
  dev->h_dev = hi;
  dev->h_cfg_eop = cfg_eop;

  dev->cfg_eop.h_eop = cfg_eop;

  dev->usb_version = dd->usb_spec;
  dev->default_iface.class_id = dd->dev_class;
  dev->default_iface.subclass_id = dd->dev_subclass;
  dev->default_iface.iface_num = 0;
  dev->default_iface.proto = dd->dev_proto;
  dev->default_iface.class = NULL;
  dev->default_iface.c_info = NULL;
  dev->default_iface.dev = dev;

  dev->cfg_eop.iface = &dev->default_iface;
  dev->cfg_eop.max_pkt = dd->max_pktsz;
  dev->cfg_eop.eop = 0;

  dev->host = h;

  dev->vendor_id = dd->vendor_id;
  dev->product_id = dd->product_id;
  dev->device_id = dd->device_id;

  if (ignore_device == false)
    {
      dev->product = usb_get_string (dev, dd->product);
      dev->manufacturer = usb_get_string (dev, dd->manufacturer);
    }

  config_val = -123;
  /* read in configuration data if there are configurations available */
  /* (which there should be...) */
  if (dd->num_configs > 0 && ignore_device == false)
    {
      config_val = usb_load_config (dev, 0, data, sizeof (data));
      if (config_val < 0)
	{
	  printf
	    ("USB: Invalid configuration value %d on '%s (%s)' (%x,%x,%x)\n",
	     config_val, dev->product, dev->manufacturer, dev->vendor_id,
	     dev->product_id, dev->device_id);
	}
    }

  usb_setup_dev_addr (dev);

  usb_config_dev (dev, (config_val < 0) ? 1 : config_val);

  return dev;

 error:
  h->dev->remove_eop (cfg_eop);
  h->dev->remove_dev_channel (hi);
  return NULL;
}

/**
 * Load in data for 'idx' configuration into device structure
 * XXX support multiple configurations
 */
static int
usb_load_config (struct usb_dev *dev, int idx, void *data, int dsz)
{
  struct usb_setup_pkt sp;
  struct config_descriptor *cd;
  struct host *h;
  host_eop_info cfg;
  void *ptr;
  int config_val, err, sz;
  int i;

  h = dev->host;
  cfg = dev->h_cfg_eop;

  sp.recipient = USB_SETUP_RECIP_DEV;
  sp.type = USB_SETUP_TYPE_STD;
  sp.direction = 1;
  sp.request = REQ_STD_GET_DESC;
  sp.value = SETUP_DESC_CONFIG << 8 | idx;
  sp.index = 0;
  sp.length = dsz;
  cd = data;

  err = h->dev->tx_pkt (cfg, USB_TOKEN_SETUP, &sp, 0, sizeof (sp), &sz, true);
  if (err != USB_HOST_ERR_NONE)
    {
      printf ("USB: Could not setup GET descriptor\n");
      return -err;
    }

  sz = usb_tx_all (&dev->cfg_eop, cd, dsz,
		   sizeof (struct config_descriptor), true);
  if (sz < (int) sizeof (struct config_descriptor))
    {
      printf ("USB: Did not rx GET descriptor (%d bytes, expected %d)\n", sz,
	      sizeof (struct config_descriptor));
      return -err;
    }

  if (sz == 0 || cd->hdr.type != SETUP_DESC_CONFIG)
    {
      printf ("USB: Invalid descriptor\n");
      return -1;
    }

  if (sz < cd->total_length)
    sz += usb_tx_all (&dev->cfg_eop, data+sz, dsz, cd->total_length - sz, true);

  send_status (h, cfg);

  dev->pwr = cd->max_power;

  /* interface information comes right after config data */
  /* scan interfaces */
  ptr = data + sizeof (struct config_descriptor);

  for (i = 0; i < cd->num_ifaces; i++)
    {
      struct interface_descriptor *iface;
      struct usb_iface *ui;
      int j;

      iface = ptr;
      if (iface->hdr.type != SETUP_DESC_IFACE)
	{
	  hex_dump (0, iface, 64, false);
	  PANIC ("Expected %d, found %d\n", SETUP_DESC_IFACE,
		 iface->hdr.type);
	}

      ui = malloc (sizeof (struct usb_iface));
      ui->class_id = iface->iface_class;
      ui->subclass_id = iface->iface_subclass;
      ui->iface_num = iface->iface_num;
      ui->proto = iface->iface_proto;
      ui->class = NULL;
      ui->c_info = NULL;
      ui->dev = dev;
      list_init (&ui->endpoints);

      /* endpoint data comes after interfaces */
      /* scan endpoints */
      ptr += sizeof (struct interface_descriptor);
      for (j = 0; j < iface->num_endpoints;
	   j++, ptr += sizeof (struct endpoint_descriptor))
	{
	  struct usb_endpoint *ue;
	  struct endpoint_descriptor *ed;

	  ed = ptr;

	  ue = malloc (sizeof (struct usb_endpoint));
	  ue->eop = ed->endpoint_num;
	  ue->direction = ed->direction;
	  ue->attr = ed->transfer;
	  ue->max_pkt = ed->max_pktsz;
	  ue->interval = ed->interval;
	  ue->iface = ui;
	  ue->h_eop = h->dev->create_eop (dev->h_dev, ue->eop, ue->max_pkt);

	  list_push_back (&ui->endpoints, &ue->peers);
	}

      list_push_back (&dev->interfaces, &ui->peers);
    }

  config_val = cd->config_val;

  return config_val;
}

/**
 * Set USB device configuration to desired configuration value
 */
static void
usb_config_dev (struct usb_dev *dev, int config_val)
{
  struct usb_setup_pkt sp;
  struct host *h;
  host_eop_info cfg;
  int err;

  h = dev->host;
  cfg = dev->h_cfg_eop;

  sp.recipient = USB_SETUP_RECIP_DEV;
  sp.type = USB_SETUP_TYPE_STD;
  sp.direction = 0;
  sp.request = REQ_STD_SET_CONFIG;
  sp.value = config_val;
  sp.index = 0;
  sp.length = 0;
  err = h->dev->tx_pkt (cfg, USB_TOKEN_SETUP, &sp, 0, sizeof (sp), NULL, true);
  if (err != USB_HOST_ERR_NONE)
    PANIC ("USB: Config setup packet did not tx\n");

  err = h->dev->tx_pkt (cfg, USB_TOKEN_IN, NULL, 0, 0, NULL, true);
  if (err != USB_HOST_ERR_NONE)
    PANIC ("USB: Could not configure device!\n");
}

/**
 * Set a device address to something other than the default pipe 
 */
static void
usb_setup_dev_addr (struct usb_dev *dev)
{
  struct usb_setup_pkt sp;
  struct host *h;
  host_eop_info cfg;
  int err;

  ASSERT (dev->addr == 0);

  h = dev->host;
  cfg = dev->h_cfg_eop;

  dev->addr = bitmap_scan_and_flip (h->usb_dev_addrs, 1, 1, false);

  sp.recipient = USB_SETUP_RECIP_DEV;
  sp.type = USB_SETUP_TYPE_STD;
  sp.direction = 0;
  sp.request = REQ_STD_SET_ADDRESS;
  sp.value = dev->addr;
  sp.index = 0;
  sp.length = 0;
  err =
    h->dev->tx_pkt (cfg, USB_TOKEN_SETUP, &sp, 0, sizeof (sp), NULL, true);
  if (err != USB_HOST_ERR_NONE)
    {
      PANIC ("USB: WHOOPS!!!!!!!\n");
    }
  err = h->dev->tx_pkt (cfg, USB_TOKEN_IN, NULL, 0, 0, NULL, true);
  if (err != USB_HOST_ERR_NONE)
    {
      PANIC ("USB: Error on setting device address (err = %d)\n", err);
    }

  h->dev->modify_dev_channel (dev->h_dev, dev->addr, dev->usb_version);
}

#define MAX_USB_STR	128
/* read string by string descriptor index from usb device */
static char *
usb_get_string (struct usb_dev *udev, int ndx)
{
  struct usb_setup_pkt sp;
  char str[MAX_USB_STR];
  char *ret;
  int sz;

  sp.recipient = USB_SETUP_RECIP_DEV;
  sp.type = USB_SETUP_TYPE_STD;
  sp.direction = 1;
  sp.request = REQ_STD_GET_DESC;
  sp.value = (SETUP_DESC_STRING << 8) | ndx;
  sp.index = 0;
  sp.length = MAX_USB_STR;

  if (udev->host->dev->tx_pkt (udev->h_cfg_eop, USB_TOKEN_SETUP,
			       &sp, 0, sizeof (sp), NULL, true) 
                           != USB_HOST_ERR_NONE) 
    return NULL;

  sz = usb_tx_all (&udev->cfg_eop, &str, MAX_USB_STR, 2, true);
  sz +=
    usb_tx_all (&udev->cfg_eop, str + sz, (uint8_t) (str[0]) - sz, 0, true);

  /* string failed to tx? */
  if (sz == 0)
    return NULL;

  send_status (udev->host, udev->h_cfg_eop);

  /* some devices don't respect the string descriptor length value (str[0]) 
   * and just send any old value they want, so we can't use it */

  str[(sz < (MAX_USB_STR - 1)) ? (sz) : (MAX_USB_STR - 1)] = '\0';

  /* usb uses wchars for strings, convert to ASCII */
  wchar_to_ascii (str, str + 2);
  ret = malloc (strlen (str) + 1);
  strlcpy (ret, str, MAX_USB_STR);
  return ret;
}

static struct class *
usb_get_class_by_id (int id)
{
  struct list_elem *li;

  li = list_begin (&class_list);
  while (li != list_end (&class_list))
    {
      struct class *c;
      c = list_entry (li, struct class, peers);
      if (c->dev->class_id == id)
	return c;
      li = list_next (li);
    }

  return NULL;
}

/**
 * Asssociate interfaces on a device with their respective device classes
 */
static void
usb_attach_interfaces (struct usb_dev *dev)
{

  struct list_elem *li;
  li = list_begin (&dev->interfaces);
  while (li != list_end (&dev->interfaces))
    {
      struct class *cl;
      struct usb_iface *ui;

      ui = list_entry (li, struct usb_iface, peers);
      li = list_next (li);
      cl = usb_get_class_by_id (ui->class_id);

      /* no matching class? try next interface */
      if (cl == NULL)
	continue;

      ui->c_info = cl->dev->attached (ui);
      /* did class driver initialize it OK? */
      if (ui->c_info != NULL)
	{
	  /* yes, add to list of usable interfaces */
	  list_push_back (&cl->usb_ifaces, &ui->class_peers);
	  ui->class = cl;
	}
    }
}

/**
 * Scan all interfaces for interfaces that match class's class id
 * Attach interfaces that match
 */
static void
usb_apply_class_to_interfaces (struct class *c)
{
  struct list_elem *li;

  li = list_begin (&usb_dev_list);
  while (li != list_end (&usb_dev_list))
    {
      struct usb_dev *ud;
      struct list_elem *lii;

      ud = list_entry (li, struct usb_dev, sys_peers);

      lii = list_begin (&ud->interfaces);
      while (lii != list_end (&ud->interfaces))
	{
	  struct usb_iface *ui;

	  ui = list_entry (lii, struct usb_iface, peers);

	  lii = list_next (lii);
	  if (ui->class_id != c->dev->class_id)
	    continue;

	  ui->c_info = c->dev->attached (ui);
	  if (ui->c_info == NULL)
	    continue;

	  list_push_back (&c->usb_ifaces, &ui->class_peers);
	  ui->class = c;
	}

      li = list_next (li);
    }
}

int
usb_dev_bulk (struct usb_endpoint *eop, void *buf, int sz, int *tx)
{
  struct host *h;
  int err;
  int token;

  ASSERT (eop != NULL);
  h = eop->iface->dev->host;

  if (eop->direction == 0)
    token = USB_TOKEN_OUT;
  else
    token = USB_TOKEN_IN;

  err = h->dev->tx_pkt (eop->h_eop, token, buf, sz, sz, tx, true);

  return err;
}

int
usb_dev_setup (struct usb_endpoint *eop, bool in,
	       struct usb_setup_pkt *s, void *buf, int sz)
{
  struct host *h;
  int err;

  ASSERT (eop != NULL);
  h = eop->iface->dev->host;

  err = h->dev->tx_pkt (eop->h_eop, USB_TOKEN_SETUP, s,
			0, sizeof (struct usb_setup_pkt), NULL, true);
  if (err != USB_HOST_ERR_NONE)
    {
      printf ("usb_dev_setup: failed\n");
      return 0;
    }

  err = h->dev->tx_pkt (eop->h_eop, (in) ? USB_TOKEN_IN : USB_TOKEN_OUT,
			buf, 0, sz, &sz, true);

  return sz;
}

/** convert a wchar string to ascii in place */
static size_t
wchar_to_ascii (char *dst, const char *src)
{
  size_t sz = 0;
  for (sz = 0; src[sz * 2] != '\0'; sz++)
    {
      dst[sz] = src[sz * 2];
    }
  dst[sz] = '\0';
  return sz;
}

/* this is used for variable sized transfers where a normal bulk transfer would 
   probably fail, since it expects some minimum size - we just want to 
   read/write as much to the pipe as we can
 */
static int
usb_tx_all (struct usb_endpoint *eop, void *buf,
	    int max_bytes, int bailout, bool in)
{
  int txed;
  int token;
  int prev_sz = 0;
  struct host *h;

  if (max_bytes <= 0)
    return 0;

  if (bailout == 0)
    bailout = 512;

  txed = 0;
  token = (in) ? USB_TOKEN_IN : USB_TOKEN_OUT;
  h = eop->iface->dev->host;
  while (txed < max_bytes && txed < bailout)
    {
      int sz, err;
      sz = 0;
      err = h->dev->tx_pkt (eop->h_eop, token,
			    buf + txed, 0, max_bytes - txed, &sz, true);
      if (prev_sz == 0)
	prev_sz = sz;
      txed += sz;
      /* this should probably be using short packet detection */
      if (err != USB_HOST_ERR_NONE || sz != prev_sz || sz == 0)
	{
	  return txed;
	}
    }
  return txed;
}
