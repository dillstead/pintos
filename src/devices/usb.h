#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <stdbool.h>
#include <kernel/list.h>

#define USB_HOST_ERR_NONE	0
#define USB_HOST_ERR_BITSTUFF	1
#define USB_HOST_ERR_TIMEOUT	2
#define USB_HOST_ERR_NAK	3
#define USB_HOST_ERR_BABBLE	4
#define USB_HOST_ERR_BUFFER	5
#define USB_HOST_ERR_STALL	6
#define USB_HOST_ERR_NODEV	7

#define make_usb_pid(x)         ((x) | ((~(x)) << 4))

/* token packets... */
#define USB_PID_OUT     make_usb_pid(1)
#define USB_PID_IN      make_usb_pid(9)
#define USB_PID_SOF     make_usb_pid(5)
#define USB_PID_SETUP   make_usb_pid(13)
/* data packets... */
#define USB_PID_DATA0   make_usb_pid(3)
#define USB_PID_DATA1   make_usb_pid(11)
#define USB_PID_DATA2   make_usb_pid(7)
#define USB_PID_MDATA   make_usb_pid(15)
/* handshake packets.. */
#define USB_PID_ACK     make_usb_pid(2)
#define USB_PID_NAK     make_usb_pid(10)
#define USB_PID_STALL   make_usb_pid(14)
#define USB_PID_NYET    make_usb_pid(6)
/* special */
#define USB_PID_PRE     make_usb_pid(12)
#define USB_PID_ERR     make_usb_pid(12)
#define USB_PID_SPLIT   make_usb_pid(8)
#define USB_PID_PING    make_usb_pid(4)

/* the standard setup requests */
#define REQ_STD_GET_STATUS      0
#define REQ_STD_CLR_FEAT        1
#define REQ_STD_SET_FEAT        3
#define REQ_STD_SET_ADDRESS     5
#define REQ_STD_GET_DESC        6
#define REQ_STD_SET_DESC        7
#define REQ_STD_GET_CONFIG      8
#define REQ_STD_SET_CONFIG      9
#define REQ_STD_GET_IFACE       10
#define REQ_STD_SET_IFACE       11
#define REQ_STD_SYNCH_FRAME     12


#define USB_TOKEN_SETUP		0x00
#define USB_TOKEN_IN		0x80
#define USB_TOKEN_OUT		0x90

struct class;
struct host;
typedef void *host_info;
typedef void *host_eop_info;
typedef void *host_dev_info;
typedef void *class_info;


struct usb_iface
{
  int iface_num;
  int class_id, subclass_id;
  int proto;

  struct usb_dev *dev;

  struct class *class;
  class_info c_info;

  struct list_elem class_peers;	/* peers on class */
  struct list endpoints;

  struct list_elem peers;	/* peers on device */
};

#define USB_SPEED_1		0
#define USB_SPEED_1_1		1
#define USB_SPEED_2		2

struct usb_host
{
  const char *name;
  int (*detect_change) (host_info);
  int (*tx_pkt) (host_eop_info, int pid, void *pkt, 
		 int min_sz, int max_sz, int *in_sz, bool wait);

  host_eop_info (*create_eop)(host_dev_info, int eop, int maxpkt);
  void (*remove_eop)(host_eop_info);

  host_dev_info (*create_dev_channel) (host_info, int dev_addr, int ver);
  void (*modify_dev_channel) (host_dev_info, int dev_addr, int ver);
  void (*remove_dev_channel) (host_dev_info);

  void (*set_toggle) (host_eop_info, int toggle);
};

struct usb_dev;
struct usb_class
{
  const int class_id;
  const char *name;

  /* when a device of this class is attached, the device is passed in */
  /* returns private info on device */
  void *(*attached) (struct usb_iface *);

  /* device is detached -> detached(dev_info) */
  void (*detached) (class_info info);
};

#define USB_VERSION_1_0	0x100
#define USB_VERSION_1_1	0x110
#define USB_VERSION_2	0x200

#define USB_EOP_ATTR_CTL	0	/* control */
#define USB_EOP_ATTR_ISO	1	/* isochronous */
#define USB_EOP_ATTR_BULK	2	/* bulk */
#define USB_EOP_ATTR_INT	3	/* interrupt */
struct usb_endpoint
{
  int eop;			/* end point address */
  int attr;
  int direction;		/* 0 = host->dev, 1=dev->host */
  int max_pkt;
  int interval;
  struct usb_iface *iface;
  host_eop_info h_eop;
  struct list_elem peers;
};

struct usb_dev
{
  uint8_t addr;
  int usb_version;
  int class_id, subclass_id;
  uint16_t product_id, vendor_id, device_id;
  int interface;
  int max_pkt_len;
  int int_period;
  char *manufacturer;
  char *product;
  int pwr;			/* power draw for this config */
  bool ignore_device;
  struct list interfaces;

  struct list_elem host_peers;	/* peers on host */
  struct list_elem sys_peers;	/* list for all devices */

  struct usb_iface default_iface;
  struct usb_endpoint cfg_eop;

  host_dev_info h_dev;
  host_eop_info h_cfg_eop;	/* configuration EOP */
  struct host *host;
};

/* pg276 usb_20.pdf */
#define USB_SETUP_TYPE_STD	0
#define USB_SETUP_TYPE_CLASS	1
#define USB_SETUP_TYPE_VENDOR	2

#define USB_SETUP_RECIP_DEV	0
#define USB_SETUP_RECIP_IFACE	1
#define USB_SETUP_RECIP_ENDPT	2
#define USB_SETUP_RECIP_OTHER	3

#pragma pack(1)
struct usb_setup_pkt
{
  uint8_t recipient:5;
  uint8_t type:2;
  uint8_t direction:1;		/* 0 = host->dev, 1 = dev->host */
  uint8_t request;
  uint16_t value;
  uint16_t index;
  uint16_t length;
};
#pragma pack()

void usb_init (void);
void usb_storage_init (void);

void usb_register_host (struct usb_host *, host_info info);
int usb_unregister_host (struct usb_host *, host_info info);
int usb_register_class (struct usb_class *);
int usb_unregister_class (struct usb_class *);

int usb_dev_bulk (struct usb_endpoint *eop, void *buf, int sz, int *tx);
int usb_dev_setup (struct usb_endpoint *eop, bool in,
		   struct usb_setup_pkt *s, void *buf, int sz);
int usb_dev_wait_int (struct usb_dev *);

#endif
