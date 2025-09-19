#ifndef UCPD_UTILS_H
#define UCPD_UTILS_H

#include <stdint.h>
#include <string.h>

/* Boolean defines */
typedef enum { UCPD_FALSE = 0u, UCPD_TRUE = 1u } UCPD_Bool;

/* Struct zero init */
#define CLEAR_STRUCT_VAL(x) memset(&(x), 0u, sizeof(x))

#define CLEAR_STRUCT_PTR(x) memset(x, 0u, sizeof(*(x)))

/* Buffer copy */

#define COPY_RAW(from, to, size) memcpy((void *)to, (const void *)from, size)

/* Voltage typedef */

typedef uint16_t UCPD_Voltage;

/* Check null */

#define CHECK_NULL(x, ret)                                                     \
  if (x == NULL) {                                                             \
    return ret;                                                                \
  }

#define CHECK_NULL_VOID(x)                                                     \
  if (x == NULL) {                                                             \
    return;                                                                    \
  }

#endif // UCPD_UTILS_H
