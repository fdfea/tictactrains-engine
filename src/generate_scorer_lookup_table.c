#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "scorer_lookup_table.h"
#include "types.h"
#include "util.h"
#include "vector.h"

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

bool BitEmpty16(uint16_t Bits);
bool BitTest16(uint16_t Bits, tIndex Index);
void BitSet16(uint16_t *pBits, tIndex Index);
void BitReset16(uint16_t *pBits, tIndex Index);

static void area_3x4_lookup_init(tArea3x4Lookup *pAreaLookup, uint16_t Data);
static void area_3x4_lookup_print(tArea3x4Lookup *pAreaLookup);
static void area_3x4_print(uint16_t Data);

static tSize area_3x4_index_longest_path(uint16_t Data, tIndex Index);
static bool area_3x4_longest_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath);
static void area_3x4_all_paths(uint16_t Data, tIndex Start, tIndex End, uint16_t Path, tVector *pPaths);
static bool area_3x4_has_through_path(uint16_t Data);
static bool area_3x4_has_path(uint16_t Data, tIndex Start, tIndex End);
static void area_3x4_get_lookup_paths(uint16_t Data, tIndex Start, tIndex End, tVector *pPaths);

int main(void)
{
    tVector Paths;
    uint16_t Data = 0x0EE8U;
    tIndex Start = 5;
    tIndex End = 3;

    vector_init(&Paths);

    printf("data:\n");
    area_3x4_print(Data);

    //area_3x4_all_paths(Data, Start, End, 0U, &Paths);
    area_3x4_get_lookup_paths(Data, Start, End, &Paths);

    for (tSize i = 0; i < vector_size(&Paths); ++i)
    {
        printf("path %u:\n", i);
        uint16_t *pData = (uint16_t *) vector_get(&Paths, i);
        area_3x4_print(*pData);
        free(pData);
    }
    
    vector_free(&Paths);

    /*
    for (uint16_t Data = 0U; Data < AREA_3X4_LOOKUP_SIZE; ++Data)
    {
        tArea3x4Lookup AreaLookup;
        
        area_3x4_lookup_init(&AreaLookup, Data);
        area_3x4_lookup_print(&AreaLookup);
    }
    */

    return 0;
}

static void area_3x4_lookup_init(tArea3x4Lookup *pAreaLookup, uint16_t Data)
{
    for (tIndex x = 0; x < AREA_3X4_INDICES; ++x)
    {
        uint16_t LongestIndexPathLength = 0;

        if (BitTest16(Data, x))
        {
            LongestIndexPathLength = area_3x4_index_longest_path(Data, x);
            //printf("longest index path length, %d: %d\n", x, LongestIndexPathLength);
        }

        pAreaLookup->IndexPaths[x].LongestPath = LongestIndexPathLength;

        for (tIndex y = 0; y < AREA_3X4_EXITS; ++y)
        {
            bool LongestExitPathFound = false;
            tIndex ExitIndex = AREA_3X4_EXIT_INDEX(y);
            tSize LongestExitPathLength = 0;
            uint16_t LongestExitPath = 0U;
        
            if (BitTest16(Data, x) AND BitTest16(Data, ExitIndex))
            {
                //if longest and left are same length, just use left?
                LongestExitPathFound = area_3x4_longest_path(Data, x, ExitIndex, &LongestExitPathLength, &LongestExitPath);
            }

            if (LongestExitPathFound)
            {
                pAreaLookup->IndexPaths[x].ExitPaths[y] = (tArea3x4PathLookup) { LongestExitPath }; 

                //printf("longest exit path, %d -> %d:\n", x, y);
                //area_3x4_print(LongestExitPath);
            }
            else
            {
                pAreaLookup->IndexPaths[x].ExitPaths[y] = (tArea3x4PathLookup) { 0U }; 
            }
        }
    }
}

static void area_3x4_lookup_print(tArea3x4Lookup *pAreaLookup)
{
    printf("{ {\n");
    for (tIndex x = 0; x < AREA_3X4_INDICES; ++x)
    {
        printf("    { { ");
        for (tIndex y = 0; y < AREA_3X4_EXITS; ++y)
        {
            char *Format = IF (y + 1 == AREA_3X4_EXITS) THEN "{0x%04xU}" ELSE "{0x%04xU}, ";
            printf(Format, pAreaLookup->IndexPaths[x].ExitPaths[y].LongestPath);
        }
        printf(" }, %d },\n", pAreaLookup->IndexPaths[x].LongestPath);
    }
    printf("} },\n");
}

static void area_3x4_print(uint16_t Data)
{
    for (tIndex i = 0; i < AREA_3X4_INDICES; ++i)
    {
        char *Format = IF ((i+1) % AREA_3X4_COLUMNS == 0) THEN "[%c]\n" ELSE "[%c]";
        char C = IF (BitTest16(Data, i)) THEN 'X' ELSE ' ';

        printf(Format, C);
    }
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
    BitReset16(&Data, Start);
    BitSet16(&Path, Start);

    if (Start == End)
    {
        uint16_t *pPath = emalloc(sizeof(uint16_t));
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
    bool ThroughPathFound = false;

    for (tIndex StartExit = 0; StartExit < AREA_3X4_EXITS; ++StartExit)
    {
        for (tIndex EndExit = 0; EndExit < AREA_3X4_EXITS; ++EndExit)
        {
            tIndex Start = AREA_3X4_EXIT_INDEX(StartExit), End = AREA_3X4_EXIT_INDEX(EndExit);

            if (StartExit != EndExit AND BitTest16(Data, Start) AND BitTest16(Data, End) AND area_3x4_has_path(Data, Start, End))
            {
                ThroughPathFound = true;
                break;
            }
        }
    }

    return ThroughPathFound;
}

static bool area_3x4_has_path(uint16_t Data, tIndex Start, tIndex End)
{
    BitReset16(&Data, Start);

    if (Start == End) return true;

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start))) area_3x4_has_path(Data, LEFT(Start), End);
    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start))) area_3x4_has_path(Data, RIGHT(Start), End);
    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start))) area_3x4_has_path(Data, TOP(Start), End);
    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start))) area_3x4_has_path(Data, BOTTOM(Start), End);

    return false;
}

static void area_3x4_get_lookup_paths(uint16_t Data, tIndex Start, tIndex End, tVector *pPaths)
{
    tVector Paths;
    uint16_t *pPath, Path;
    tSize PathLength;

    vector_init(&Paths);

    if (area_3x4_longest_path(Data, Start, End, &PathLength, &Path))
    {
        pPath = emalloc(sizeof(uint16_t));
        *pPath = Path;
        vector_add(pPaths, (void *) pPath);
    }

    area_3x4_all_paths(Data, Start, End, 0U, &Paths);

    while ((pPath = (uint16_t *) vector_pop(&Paths)) ISNOT NULL)
    {
        if (area_3x4_has_through_path(Data & ~*pPath))
        {
            vector_add(pPaths, (void *) pPath);
        }
        else
        {
            free(pPath);
        }
    }

    vector_free(&Paths);
}

bool BitEmpty16(uint16_t Bits)
{
    return Bits == 0U;
}

bool BitTest16(uint16_t Bits, tIndex Index)
{
    return NOT BitEmpty16(Bits & (1U << Index));
}

void BitSet16(uint16_t *pBits, tIndex Index)
{
    *pBits |= (1U << Index);
}

void BitReset16(uint16_t *pBits, tIndex Index)
{
    *pBits &= ~(1U << Index);
}
