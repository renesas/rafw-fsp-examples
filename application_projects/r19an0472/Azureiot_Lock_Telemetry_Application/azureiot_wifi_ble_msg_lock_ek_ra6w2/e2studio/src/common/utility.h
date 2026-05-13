#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>

#define ARRAY_SIZE(x) sizeof(x) / sizeof(x[0])
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define UNUSED(x) (void) (x)

int32_t string_to_ip(const char *ip_str, uint8_t *binary_ip);
const char* ip_to_string(uint32_t binary_ip, const char *ip_str, uint32_t len);

#endif /* __COMMON_H__ */
