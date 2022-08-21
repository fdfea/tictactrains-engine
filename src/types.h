#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>

#define TINDEX_MAX  UINT8_MAX
#define TSIZE_MAX   UINT8_MAX
#define TSCORE_MAX  INT8_MAX

#ifdef PACKED
typedef uint8_t tIndex;
typedef uint8_t tSize;
typedef int8_t tScore;
#else
typedef uint_fast8_t tIndex;
typedef uint_fast8_t tSize;
typedef int_fast8_t tScore;
#endif

#endif
