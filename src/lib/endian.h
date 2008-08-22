#ifndef __LIB_ENDIAN_H
#define __LIB_ENDIAN_H


#define endian_swap32(x)       ((((x) & 0xff000000) >> 24) | \
		                   (((x) & 0x00ff0000) >> 8)  | \
		                   (((x) & 0x0000ff00) << 8)  | \
		                   (((x) & 0x000000ff) << 24))
#define endian_swap16(x)        ((((x) & 0xff00)>> 8) | (((x) & 0x00ff) << 8))
#define endian_swap24(x)	(((x) >> 16) & 0xff) | (((x) & 0xff) << 16) | ((x) & 0xff00)

#define be32_to_machine(x)	endian_swap32(x)
#define be16_to_machine(x)	endian_swap16(x)
#define be24_to_machine(x)	endian_swap24(x)
#define machine_to_be32(x)	endian_swap32(x)
#define machine_to_be16(x)	endian_swap16(x)
#define machine_to_be24(x)	endian_swap24(x)

#endif
