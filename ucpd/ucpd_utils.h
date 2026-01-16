#ifndef UCPD_UTILS_H
#define UCPD_UTILS_H

#include <stdbool.h>
#include <string.h>

/* Buffer copy */

#define COPY_RAW(from, to, size) memcpy((void*)to, (const void*)from, size)

#endif // UCPD_UTILS_H
