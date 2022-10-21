#ifndef __SCORER_LOOKUP_TABLE_H__
#define __SCORER_LOOKUP_TABLE_H__

#include <stdint.h>

#include "types.h"
#include "vector.h"

#define AREA_3X4_ROWS           3
#define AREA_3X4_COLUMNS        4
#define AREA_3X4_INDICES        (AREA_3X4_ROWS * AREA_3X4_COLUMNS)
#define AREA_3X4_EXITS          7
#define AREA_3X4_MASK           0x0FFFU
#define AREA_3X4_LOOKUP_SIZE    4096

typedef struct Area3x4Path
{
    uint64_t Path;
    tSize Length;
}
tArea3x4Path;

typedef struct Area3x4PathLookup
{
    tVector Paths;
    tIndex Exit;
}
tArea3x4PathLookup;

typedef struct Area3x4IndexLookup
{
    tVector Exits;
    tSize LongestPath;
}
tArea3x4IndexLookup;

typedef struct Area3x4Lookup
{
    tArea3x4IndexLookup Indices[AREA_3X4_INDICES];
}
tArea3x4Lookup;

void scorer_lookup_init(void);
void scorer_lookup_free(void);

void area_3x4_print(uint16_t Data);

tArea3x4Lookup *scorer_area_3x4_lookup(uint16_t Data);

#endif
