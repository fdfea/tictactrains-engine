#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdio.h>

#include "bitutil.h"
#include "vector.h"
#include "scorer_lookup_table.h"
#include "types.h"
#include "util.h"

typedef struct Area3x4AjacentIndexLookup
{
    bool LeftValid, RightValid, TopValid, BottomValid;
    tIndex Left, Right, Top, Bottom;
}
tArea3x4AdjacentIndexLookup;

static const tArea3x4AdjacentIndexLookup Area3x4AdjacentIndexLookup[AREA_3X4_INDICES] = {
    { false, true , false, true ,  0,  1,  0,  4 },
    { true , true , false, true ,  0,  2,  0,  5 },
    { true , true , false, true ,  1,  3,  0,  6 },
    { true , false, false, true ,  2,  0,  0,  7 },
    { false, true , true , true ,  0,  5,  0,  8 },
    { true , true , true , true ,  4,  6,  1,  9 },
    { true , true , true , true ,  5,  7,  2, 10 },
    { true , false, true , true ,  6,  0,  3, 11 },
    { false, true , true , false,  0,  9,  4,  0 },
    { true , true , true , false,  8, 10,  5,  0 },
    { true , true , true , false,  9, 11,  6,  0 },
    { true , false, true , false, 10,  0,  7,  0 },
};

#define LEFT_VALID(i)       Area3x4AdjacentIndexLookup[i].LeftValid
#define RIGHT_VALID(i)      Area3x4AdjacentIndexLookup[i].RightValid
#define TOP_VALID(i)        Area3x4AdjacentIndexLookup[i].TopValid
#define BOTTOM_VALID(i)     Area3x4AdjacentIndexLookup[i].BottomValid

#define LEFT(i)         Area3x4AdjacentIndexLookup[i].Left
#define RIGHT(i)        Area3x4AdjacentIndexLookup[i].Right
#define TOP(i)          Area3x4AdjacentIndexLookup[i].Top
#define BOTTOM(i)       Area3x4AdjacentIndexLookup[i].Bottom

static const tIndex Area3x4ExitIndexLookup[AREA_3X4_EXITS] = { 3, 7, 11, 8, 9, 10, 11 };

#define AREA_3X4_EXIT_INDEX(i)      Area3x4ExitIndexLookup[i]

static tArea3x4Lookup Area3x4Lookup[AREA_3X4_LOOKUP_SIZE] = {{{{{{ 0 }}}}}};

static tSize area_3x4_index_longest_path(uint16_t Data, tIndex Index);
static bool area_3x4_longest_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath);
static void area_3x4_all_paths(uint16_t Data, tIndex Start, tIndex End, uint16_t Path, tVector *pPaths);
static bool area_3x4_has_through_path(uint16_t Data);
static bool area_3x4_has_path(uint16_t Data, tIndex Start, tIndex End);
static uint16_t *area_3x4_get_lookup_paths(uint16_t Data, tIndex Start, tIndex End, tSize *pSize);

static void area_3x4_lookup_init(tArea3x4Lookup *pLookup, uint16_t Data);
static void area_3x4_lookup_print(tArea3x4Lookup *pLookup);
//static void area_3x4_print(uint16_t Data);

/*
int main(void)
{
    uint16_t Data = 0x0CD5U;
    tIndex Start = 7;
    tIndex End = 11;
    tSize Size;
    uint16_t Path;

    printf("data:\n");
    area_3x4_print(Data);

    tArea3x4Lookup Lookup;

    //area_3x4_lookup_init(&Lookup, Data);
    //area_3x4_lookup_print(&Lookup);

    //bool PathFound = area_3x4_longest_path(Data, Start, End, &Size, &Path);

    //printf("path found: %d\n", PathFound);
    //printf("path:\n");
    //area_3x4_print(Path);

    uint16_t *pPaths = area_3x4_get_lookup_paths(Data, Start, End, &Size);

    for (tSize i = 0; i < Size; ++i)
    {
        printf("path %u:\n", i);
        uint16_t Path = pPaths[i];
        area_3x4_print(Path);
    }

    free(pPaths);

    return 0;
}
*/

void area_3x4_print(uint16_t Data)
{
    for (tIndex i = 0; i < AREA_3X4_INDICES; ++i)
    {
        char *Format = IF ((i+1) % AREA_3X4_COLUMNS == 0) THEN "[%c]\n" ELSE "[%c]";
        char C = IF (BitTest16(Data, i)) THEN 'X' ELSE ' ';

        printf(Format, C);
    }
}

static void area_3x4_lookup_init(tArea3x4Lookup *pLookup, uint16_t Data)
{
    for (tIndex i = 0; i < AREA_3X4_INDICES; ++i)
    {
        for (tIndex j = 0; j < AREA_3X4_EXITS; ++j)
        {
            tSize Size;

            pLookup->IndexPaths[i].ExitPaths[j].Paths = area_3x4_get_lookup_paths(Data, i, AREA_3X4_EXIT_INDEX(j), &Size);
            pLookup->IndexPaths[i].ExitPaths[j].Size = Size;
        }
        pLookup->IndexPaths[i].LongestPath = IF BitTest16(Data, i) THEN area_3x4_index_longest_path(Data, i) ELSE 0;
    }
}

static void area_3x4_lookup_print(tArea3x4Lookup *pLookup)
{
    for (tIndex i = 0; i < AREA_3X4_INDICES; ++i)
    {
        printf("LONGEST PATH %d: %d\n", i, pLookup->IndexPaths[i].LongestPath);

        for (tIndex j = 0; j < AREA_3X4_EXITS; ++j)
        {
            tArea3x4PathLookup *pPathLookup = &pLookup->IndexPaths[i].ExitPaths[j];

            printf("PATHS %d -> %d:\n", i, AREA_3X4_EXIT_INDEX(j));

            for (tIndex k = 0; k < pPathLookup->Size; ++k)
            {
                uint16_t Path = pPathLookup->Paths[k];

                area_3x4_print(Path);
                printf("\n");
            }
        }
    }
}

// use rotations to reduce creation time
void scorer_area_3x4_lookup_init(void)
{
    for (uint16_t i = 0; i < AREA_3X4_LOOKUP_SIZE; ++i)
    {
        for (tIndex j = 0; j < AREA_3X4_INDICES; ++j)
        {
            for (tIndex k = 0; k < AREA_3X4_EXITS; ++k)
            {
                tSize Size;

                Area3x4Lookup[i].IndexPaths[j].ExitPaths[k].Paths = area_3x4_get_lookup_paths(i, j, AREA_3X4_EXIT_INDEX(k), &Size);
                Area3x4Lookup[i].IndexPaths[j].ExitPaths[k].Size = Size;
            }
            Area3x4Lookup[i].IndexPaths[j].LongestPath = IF BitTest16(i, j) THEN area_3x4_index_longest_path(i, j) ELSE 0;
        }
    }
}

void scorer_area_3x4_lookup_free(void)
{
    for (uint16_t i = 0; i < AREA_3X4_LOOKUP_SIZE; ++i)
    {
        for (tIndex j = 0; j < AREA_3X4_INDICES; ++j)
        {
            for (tIndex k = 0; k < AREA_3X4_EXITS; ++k)
            {
                free(Area3x4Lookup[i].IndexPaths[j].ExitPaths[k].Paths);
            }
        }
    }
}

const tArea3x4Lookup *scorer_area_3x4_lookup(uint16_t Data)
{
    return &Area3x4Lookup[Data & AREA_3X4_MASK];
}

static tSize area_3x4_index_longest_path(uint16_t Data, tIndex Index)
{
    tSize PathLength, MaxPathLength = 0;

    BitReset16(&Data, Index);

    if (LEFT_VALID(Index) AND BitTest16(Data, LEFT(Index)))
    {
        PathLength = area_3x4_index_longest_path(Data, LEFT(Index));
        SET_IF_GREATER(PathLength, MaxPathLength);
    }

    if (RIGHT_VALID(Index) AND BitTest16(Data, RIGHT(Index)))
    {
        PathLength = area_3x4_index_longest_path(Data, RIGHT(Index));
        SET_IF_GREATER(PathLength, MaxPathLength);
    }

    if (TOP_VALID(Index) AND BitTest16(Data, TOP(Index)))
    {
        PathLength = area_3x4_index_longest_path(Data, TOP(Index));
        SET_IF_GREATER(PathLength, MaxPathLength);
    }

    if (BOTTOM_VALID(Index) AND BitTest16(Data, BOTTOM(Index)))
    {
        PathLength = area_3x4_index_longest_path(Data, BOTTOM(Index));
        SET_IF_GREATER(PathLength, MaxPathLength);
    }

    return MaxPathLength + 1;
}

static bool area_3x4_longest_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath)
{
    tSize PathLength, MaxPathLength = 0;
    uint16_t MaxPath, ThisPath = 0U;
    bool PathFound = false;

    BitReset16(&Data, Start);
    BitSet16(&ThisPath, Start);

    MaxPath = ThisPath;

    if (Start == End)
    {
        PathFound = true;
        goto Done;
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_longest_path(Data, LEFT(Start), End, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
        }
    }

    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_longest_path(Data, RIGHT(Start), End, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
        }
    }

    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_longest_path(Data, TOP(Start), End, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
        }
    }

    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_longest_path(Data, BOTTOM(Start), End, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
        }
    }

Done:
    *pPath |= MaxPath;
    *pLength = MaxPathLength + 1;

    return PathFound;
}

static void area_3x4_all_paths(uint16_t Data, tIndex Start, tIndex End, uint16_t Path, tVector *pPaths)
{
    uint16_t *pPath;
    
    BitReset16(&Data, Start);
    BitSet16(&Path, Start);

    if (Start == End)
    {
        pPath = emalloc(sizeof(uint16_t));
        *pPath = Path;

        vector_add(pPaths, (void *) pPath);

        return;
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start))) area_3x4_all_paths(Data, LEFT(Start), End, Path, pPaths);
    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start))) area_3x4_all_paths(Data, RIGHT(Start), End, Path, pPaths);
    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start))) area_3x4_all_paths(Data, TOP(Start), End, Path, pPaths);
    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start))) area_3x4_all_paths(Data, BOTTOM(Start), End, Path, pPaths);
}

static bool area_3x4_has_through_path(uint16_t Data)
{
    bool PathFound = false;

    for (tIndex StartExit= 0; StartExit < AREA_3X4_EXITS; ++StartExit)
    {
        for (tIndex End = 0; End < AREA_3X4_INDICES; ++End)
        {
            tIndex Start = AREA_3X4_EXIT_INDEX(StartExit);

            if (/*Start != End AND*/ BitTest16(Data, Start) AND BitTest16(Data, End) AND area_3x4_has_path(Data, Start, End))
            {
                PathFound = true;
                break;
            }
        }
    }

    return PathFound;
}

static bool area_3x4_has_path(uint16_t Data, tIndex Start, tIndex End)
{
    bool PathFound = false;

    BitReset16(&Data, Start);

    if (Start == End)
    {
        PathFound = true;
        goto Done;
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start)) AND (PathFound = area_3x4_has_path(Data, LEFT(Start), End))) goto Done;
    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start)) AND (PathFound = area_3x4_has_path(Data, RIGHT(Start), End))) goto Done;
    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start)) AND (PathFound = area_3x4_has_path(Data, TOP(Start), End))) goto Done;
    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start)) AND (PathFound = area_3x4_has_path(Data, BOTTOM(Start), End))) goto Done;

Done:
    return PathFound;
}

static uint16_t *area_3x4_get_lookup_paths(uint16_t Data, tIndex Start, tIndex End, tSize *pSize)
{
    tVector AllPaths, LookupPaths;
    uint16_t *pPath, *Paths = NULL, Path = 0U;
    tSize PathLength;

    vector_init(&AllPaths);
    vector_init(&LookupPaths);

    if (NOT BitTest16(Data, Start) OR NOT BitTest16(Data, End))
    {
        goto Error;
    }

    if (area_3x4_longest_path(Data, Start, End, &PathLength, &Path))
    {
        pPath = emalloc(sizeof(uint16_t));
        *pPath = Path;

        vector_add(&LookupPaths, (void *) pPath);
    }
    else
    {
        goto Error;
    }

    area_3x4_all_paths(Data, Start, End, 0U, &AllPaths);

    while ((pPath = (uint16_t *) vector_pop(&AllPaths)) ISNOT NULL)
    {
        if (area_3x4_has_through_path(Data & ~*pPath))
        {
            vector_add(&LookupPaths, (void *) pPath);
        }
        else
        {
            free(pPath);
        }
    }

    Paths = emalloc(vector_size(&LookupPaths) * sizeof(uint16_t));

    for (tIndex i = 0; i < vector_size(&LookupPaths); ++i)
    {
        pPath = (uint16_t *) vector_get(&LookupPaths, i);
        Paths[i] = *pPath;

        free(pPath);
    }

Error:
    *pSize = vector_size(&LookupPaths);

    vector_free(&AllPaths);
    vector_free(&LookupPaths);

    return Paths;
}
