#ifndef DEVICE_RESOURCE_H
#define DEVICE_RESOURCE_H

struct resource
{
  uint32_t start;
  uint32_t end;
  uint8_t type;
};

#define RESOURCE_TYPE_MEM 0x0
#define RESOURCE_TYPE_IO 0x1

#endif /* device/resource.h */
