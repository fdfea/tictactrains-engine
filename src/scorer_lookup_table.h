#ifndef __SCORER_LOOKUP_TABLE_H__
#define __SCORER_LOOKUP_TABLE_H__

#include <stdint.h>

#include "types.h"

#define AREA_3X4_ROWS       3
#define AREA_3X4_COLUMNS    4
#define AREA_3X4_INDICES    (AREA_3X4_ROWS * AREA_3X4_COLUMNS)
#define AREA_3X4_EXITS      7
#define AREA_3X4_MASK  0x0FFFU

#define AREA_3X4_LOOKUP_SIZE  4096

typedef struct Area3x4PathLookup
{
    uint16_t LongestPath, LeastExitPath, ShortestPath;
}
tArea3x4PathLookup;

typedef struct Area3x4IndexLookup
{
    tArea3x4PathLookup ExitPaths[AREA_3X4_EXITS];
    tSize LongestPath;
}
tArea3x4IndexLookup;

typedef struct Area3x4Lookup
{
    tArea3x4IndexLookup IndexPaths[AREA_3X4_INDICES];
}
tArea3x4Lookup;

const tArea3x4Lookup *scorer_area_3x4_lookup(uint16_t Data);

#endif
