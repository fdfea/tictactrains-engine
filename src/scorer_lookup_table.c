#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdio.h>

#include "bitutil.h"
#include "board.h"
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

static tArea3x4Lookup Area3x4Lookup[AREA_3X4_LOOKUP_SIZE] = { 0 };

static tSize area_3x4_index_longest_path(uint16_t Data, tIndex Index);
static bool area_3x4_longest_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath);
static bool area_3x4_has_through_path(uint16_t Data);
static bool area_3x4_has_path(uint16_t Data, tIndex Start, tIndex End);
static void area_3x4_get_lookup_paths(uint16_t Data, tIndex Start, tIndex End, uint16_t Path, tVector *pVector);

void scorer_lookup_init(void)
{
    for (uint16_t Data = 0; Data < AREA_3X4_LOOKUP_SIZE; ++Data)
    {
        tArea3x4Lookup *pLookup = &Area3x4Lookup[Data];

        for (tIndex Start = 0; Start < AREA_3X4_INDICES; ++Start)
        {
            tArea3x4IndexLookup *pIndexLookup = &pLookup->Indices[Start];
                
            if (BitTest16(Data, Start))
            {
                vector_init(&pIndexLookup->Exits);

                pIndexLookup->LongestPath = area_3x4_index_longest_path(Data, Start);

                for (tIndex Exit = 0; Exit < AREA_3X4_EXITS; ++Exit)
                {
                    tIndex End = AREA_3X4_EXIT_INDEX(Exit);
                    uint16_t Path = 0U;
                    tSize Length;

                    if (BitTest16(Data, End) AND area_3x4_longest_path(Data, Start, End, &Length, &Path))
                    {
                        tArea3x4PathLookup *pPathLookup = emalloc(sizeof(tArea3x4PathLookup));

                        vector_init(&pPathLookup->Paths);
                        area_3x4_get_lookup_paths(Data, Start, End, 0U, &pPathLookup->Paths);

                        pPathLookup->Exit = Exit;

                        tVectorIterator PathsIterator;

                        vector_iterator_init(&PathsIterator, &pPathLookup->Paths);

                        bool DuplicatePath = false;

                        while (vector_iterator_has_next(&PathsIterator))
                        {
                            tArea3x4Path *pPath = vector_iterator_next(&PathsIterator);

                            if (pPath->Path == Path)
                            {
                                DuplicatePath = true;
                                break;
                            }
                        }

                        if (NOT DuplicatePath)
                        {
                            tArea3x4Path *pPath;

                            pPath = emalloc(sizeof(tArea3x4Path));
                            pPath->Path = Path;
                            pPath->Length = Length;

                            vector_add(&pPathLookup->Paths, (void *) pPath);
                        }
                            
                        vector_add(&pIndexLookup->Exits, (void *) pPathLookup);
                    }
                }
            }
        }
    }
}

void scorer_lookup_free(void)
{
    for (uint16_t Data = 0; Data < AREA_3X4_LOOKUP_SIZE; ++Data)
    {
        tArea3x4Lookup *pLookup = &Area3x4Lookup[Data];

        for (tIndex i = 0; i < AREA_3X4_INDICES; ++i)
        {
            tArea3x4IndexLookup *pIndexLookup = &pLookup->Indices[i];
            tVectorIterator ExitsIterator;

            vector_iterator_init(&ExitsIterator, &pIndexLookup->Exits);

            while (vector_iterator_has_next(&ExitsIterator))
            {
                tArea3x4PathLookup *pPaths = (tArea3x4PathLookup *) vector_iterator_next(&ExitsIterator);
                tVectorIterator PathsIterator;

                vector_iterator_init(&PathsIterator, &pPaths->Paths);

                while (vector_iterator_has_next(&PathsIterator))
                {
                    tArea3x4Path *pPath = (tArea3x4Path *) vector_iterator_next(&PathsIterator);

                    free(pPath);
                }

                vector_free(&pPaths->Paths);
                free(pPaths);
            }

            vector_free(&pIndexLookup->Exits);
        }
    }
}

tArea3x4Lookup *scorer_area_3x4_lookup(uint16_t Data)
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

static void area_3x4_get_lookup_paths(uint16_t Data, tIndex Start, tIndex End, uint16_t Path, tVector *pPaths)
{
    BitSet16(&Path, Start);
    BitReset16(&Data, Start);

    if (Start == End AND area_3x4_has_through_path(Data & ~Path))
    {
        tArea3x4Path *pPath = emalloc(sizeof(tArea3x4Path));

        pPath->Path = Path;
        pPath->Length = BitPopCount16(Path);

        vector_add(pPaths, (void *) pPath);

        return;
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start))) area_3x4_get_lookup_paths(Data, LEFT(Start), End, Path, pPaths);
    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start))) area_3x4_get_lookup_paths(Data, RIGHT(Start), End, Path, pPaths);
    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start))) area_3x4_get_lookup_paths(Data, TOP(Start), End, Path, pPaths);
    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start))) area_3x4_get_lookup_paths(Data, BOTTOM(Start), End, Path, pPaths);
}

static bool area_3x4_has_through_path(uint16_t Data)
{
    bool PathFound = false;

    for (tIndex StartExit= 0; StartExit < AREA_3X4_EXITS; ++StartExit)
    {
        for (tIndex End = 0; End < AREA_3X4_INDICES; ++End)
        {
            tIndex Start = AREA_3X4_EXIT_INDEX(StartExit);

            if (BitTest16(Data, Start) AND BitTest16(Data, End) AND area_3x4_has_path(Data, Start, End))
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