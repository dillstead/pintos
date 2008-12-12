#include "devices/usb.h"
#include "devices/usb_hub.h"
#include <lib/debug.h>

#define USB_CLASS_HUB	0x09

#define REQ_HUB_GET_STATUS	0
#define REQ_HUB_CLEAR_FEATURE	1
#define REQ_HUB_SET_FEATURE	2
#define REQ_HUB_GET_DESC	6
#define REQ_HUB_SET_DESC	7
#define REQ_HUB_CLEAR_TT_BUF	8
#define REQ_HUB_RESET_TT	9
#define REQ_HUB_GET_TT_STATE	10
#define REQ_HUB_STOP_TT		11

#define HUB_SEL_HUB_POWER		0
#define HUB_SEL_OVER_CURRENT		1
#define HUB_SEL_PORT_CONN		0
#define HUB_SEL_PORT_ENABLE		1
#define HUB_SEL_PORT_SUSPEND		2
#define HUB_SEL_PORT_OVER_CURRENT	3
#define HUB_SEL_PORT_RESET		4
#define HUB_SEL_PORT_POWER		8
#define HUB_SEL_PORT_LOW_SPEED		9

#define SETUP_DESC_HUB			0x29

static void* hub_attached(struct usb_iface*);
static void hub_detached(class_info);

static struct usb_class hub_class = {
		.attached = hub_attached,
		.detached = hub_detached,
		.class_id = USB_CLASS_HUB,
		.name = "hub"
		};


void usb_hub_init(void)
{
	usb_register_class(&hub_class);
}


static void* hub_attached(struct usb_iface* ui UNUSED)
{
	return NULL;
}

static void hub_detached(class_info info UNUSED)
{
}

