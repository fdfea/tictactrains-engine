#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "bitutil.h"
#include "board.h"
#include "scorer.h"
#include "types.h"
#include "util.h"
#include "vector.h"

#define AREA_3X4_INDICES        12
#define AREA_3X4_EXITS          7
#define AREA_3X4_MASK           0x0FFFU
#define AREA_3X4_LOOKUP_SIZE    4096

#define AREA_1X1_EXITS      4
#define AREA_1X1_INDICES    1
#define AREA_1X1_MASK       0x0001U

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

typedef struct Area3x4AjacentIndexLookup
{
    bool LeftValid, RightValid, TopValid, BottomValid;
    tIndex Left, Right, Top, Bottom;
}
tArea3x4AdjacentIndexLookup;

static const tIndex BoardAreaExitIndicesQ0[AREA_1X1_EXITS] = { 17, 25, 31, 23 };
static const tIndex BoardAreaExitIndicesQ1[AREA_3X4_EXITS] = {  4, 11, 18, 21, 22, 23, 24 };
static const tIndex BoardAreaExitIndicesQ2[AREA_3X4_EXITS] = { 34, 33, 32,  3, 10, 17, 24 };
static const tIndex BoardAreaExitIndicesQ3[AREA_3X4_EXITS] = { 44, 37, 30, 27, 26, 25, 24 };
static const tIndex BoardAreaExitIndicesQ4[AREA_3X4_EXITS] = { 14, 15, 16, 45, 38, 31, 24 };

static tArea3x4Lookup Area3x4Lookup[AREA_3X4_LOOKUP_SIZE] = { 0 };
static const tIndex Area3x4ExitIndexLookup[AREA_3X4_EXITS] = { 3, 7, 11, 8, 9, 10, 11 };

static const tIndex board_area_index_c[ROWS*COLUMNS] = {
     0,  1,  2,  3,  8,  4,  0, 
     4,  5,  6,  7,  9,  5,  1,
     8,  9, 10, 11, 10,  6,  2,
     3,  7, 11,  0, 11,  7,  3,
     2,  6, 10, 11, 10,  9,  8,
     1,  5,  9,  7,  6,  5,  4, 
     0,  4,  8,  3,  2,  1,  0,
};

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

#define AREA_3X4_EXIT_INDEX(i)      Area3x4ExitIndexLookup[i]

static tSize scorer_lookup_longest_path(uint64_t Data, tIndex Index);
static tSize area_3x4_lookup_longest_path(uint64_t Data, tIndex Index);
static tSize area_1x1_lookup_longest_path(uint64_t Data, tIndex Index);

static tSize area_3x4_index_longest_path(uint16_t Data, tIndex Index);
static bool area_3x4_longest_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath);
static bool area_3x4_has_through_path(uint16_t Data);
static bool area_3x4_has_path(uint16_t Data, tIndex Start, tIndex End);
static void area_3x4_get_lookup_paths(uint16_t Data, tIndex Start, tIndex End, uint16_t Path, tVector *pVector);

static uint16_t extract_q0(uint64_t Data);
static uint16_t extract_q1(uint64_t Data);
static uint16_t extract_q2(uint64_t Data);
static uint16_t extract_q3(uint64_t Data);
static uint16_t extract_q4(uint64_t Data);

static uint64_t convert_q0(uint16_t Data);
static uint64_t convert_q1(uint16_t Data);
static uint64_t convert_q2(uint16_t Data);
static uint64_t convert_q3(uint16_t Data);
static uint64_t convert_q4(uint16_t Data);

static tIndex board_area_exit_index_q0(tIndex Index);
static tIndex board_area_exit_index_q1(tIndex Index);
static tIndex board_area_exit_index_q2(tIndex Index);
static tIndex board_area_exit_index_q3(tIndex Index);
static tIndex board_area_exit_index_q4(tIndex Index);

static const uint16_t (*extract_q[ROWS*COLUMNS])(uint64_t) = {
    extract_q1, extract_q1, extract_q1, extract_q1, extract_q2, extract_q2, extract_q2,
    extract_q1, extract_q1, extract_q1, extract_q1, extract_q2, extract_q2, extract_q2,
    extract_q1, extract_q1, extract_q1, extract_q1, extract_q2, extract_q2, extract_q2,
    extract_q4, extract_q4, extract_q4, extract_q0, extract_q2, extract_q2, extract_q2,
    extract_q4, extract_q4, extract_q4, extract_q3, extract_q3, extract_q3, extract_q3,
    extract_q4, extract_q4, extract_q4, extract_q3, extract_q3, extract_q3, extract_q3,
    extract_q4, extract_q4, extract_q4, extract_q3, extract_q3, extract_q3, extract_q3,
};

static const uint64_t (*convert_q[ROWS*COLUMNS])(uint16_t) = {
    convert_q1, convert_q1, convert_q1, convert_q1, convert_q2, convert_q2, convert_q2,
    convert_q1, convert_q1, convert_q1, convert_q1, convert_q2, convert_q2, convert_q2,
    convert_q1, convert_q1, convert_q1, convert_q1, convert_q2, convert_q2, convert_q2,
    convert_q4, convert_q4, convert_q4, convert_q0, convert_q2, convert_q2, convert_q2,
    convert_q4, convert_q4, convert_q4, convert_q3, convert_q3, convert_q3, convert_q3,
    convert_q4, convert_q4, convert_q4, convert_q3, convert_q3, convert_q3, convert_q3,
    convert_q4, convert_q4, convert_q4, convert_q3, convert_q3, convert_q3, convert_q3,
};

static const tIndex (*board_area_exit_index_q[ROWS*COLUMNS])(tIndex) = {
    board_area_exit_index_q1, board_area_exit_index_q1, board_area_exit_index_q1, board_area_exit_index_q1, board_area_exit_index_q2, board_area_exit_index_q2, board_area_exit_index_q2,
    board_area_exit_index_q1, board_area_exit_index_q1, board_area_exit_index_q1, board_area_exit_index_q1, board_area_exit_index_q2, board_area_exit_index_q2, board_area_exit_index_q2,
    board_area_exit_index_q1, board_area_exit_index_q1, board_area_exit_index_q1, board_area_exit_index_q1, board_area_exit_index_q2, board_area_exit_index_q2, board_area_exit_index_q2,
    board_area_exit_index_q4, board_area_exit_index_q4, board_area_exit_index_q4, board_area_exit_index_q0, board_area_exit_index_q2, board_area_exit_index_q2, board_area_exit_index_q2,
    board_area_exit_index_q4, board_area_exit_index_q4, board_area_exit_index_q4, board_area_exit_index_q3, board_area_exit_index_q3, board_area_exit_index_q3, board_area_exit_index_q3,
    board_area_exit_index_q4, board_area_exit_index_q4, board_area_exit_index_q4, board_area_exit_index_q3, board_area_exit_index_q3, board_area_exit_index_q3, board_area_exit_index_q3,
    board_area_exit_index_q4, board_area_exit_index_q4, board_area_exit_index_q4, board_area_exit_index_q3, board_area_exit_index_q3, board_area_exit_index_q3, board_area_exit_index_q3,
};

static const tSize (*area_lookup_longest_path[ROWS*COLUMNS])(uint64_t, tIndex) = {
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_1x1_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
};

void scorer_init()
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

void scorer_free()
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

tScore scorer_score(tBoard *pBoard)
{
    tScore ScoreX = 0, ScoreO = 0;

    for (tIndex Index = 0; Index < ROWS*COLUMNS; ++Index)
    {
        bool Player = board_index_player(pBoard, Index);
        uint64_t Data = IF Player THEN pBoard->Data ELSE ~pBoard->Data & BOARD_MASK;
        tScore Score = scorer_lookup_longest_path(Data, Index);

        if (Player)
        {
            SET_IF_GREATER(Score, ScoreX);
        }
        else
        {
            SET_IF_GREATER(Score, ScoreO);
        }
    }

    return ScoreX - ScoreO;
}

static tSize scorer_lookup_longest_path(uint64_t Data, tIndex Index)
{
    return (*area_lookup_longest_path[Index])(Data, Index);
}

static tSize area_3x4_lookup_longest_path(uint64_t Data, tIndex Index)
{
    tArea3x4Lookup *pLookup = &Area3x4Lookup[((*extract_q[Index])(Data))];
    tArea3x4IndexLookup *pIndexLookup = &pLookup->Indices[board_area_index_c[Index]];
    tSize MaxPathLength = 0;

    SET_IF_GREATER(pIndexLookup->LongestPath, MaxPathLength);

    tVector *pExitLookup = &pIndexLookup->Exits;
    tVectorIterator ExitsIterator;

    vector_iterator_init(&ExitsIterator, pExitLookup);

    while (vector_iterator_has_next(&ExitsIterator))
    {
        tArea3x4PathLookup *pPathLookup = (tArea3x4PathLookup *) vector_iterator_next(&ExitsIterator);
        tIndex NextIndex = (*board_area_exit_index_q[Index])(pPathLookup->Exit);

        if (BitTest64(Data, NextIndex))
        {
            tVector *pPaths = &pPathLookup->Paths;
            tVectorIterator PathsIterator;

            vector_iterator_init(&PathsIterator, pPaths);

            while (vector_iterator_has_next(&PathsIterator))
            {
                tArea3x4Path *pPath = (tArea3x4Path *) vector_iterator_next(&PathsIterator);
                uint64_t PathConverted = (*convert_q[Index])(pPath->Path);
                tSize LookupLength = pPath->Length + scorer_lookup_longest_path(Data & ~PathConverted, NextIndex);

                SET_IF_GREATER(LookupLength, MaxPathLength);
            }
        }
    }

    return MaxPathLength;
}

static tSize area_1x1_lookup_longest_path(uint64_t Data, tIndex Index)
{
    tSize MaxPathLength = 0;

    BitReset64(&Data, Index);

    for (tIndex i = 0; i < AREA_1X1_EXITS; ++i)
    {
        tIndex NextIndex = (*board_area_exit_index_q[Index])(i);

        if (BitTest64(Data, NextIndex))
        {
            tSize PathLength = scorer_lookup_longest_path(Data, NextIndex);
            SET_IF_GREATER(PathLength, MaxPathLength);
        }
    }

    return 1 + MaxPathLength;
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

static uint16_t extract_q0(uint64_t Data)
{
    uint16_t Q = 0U;

    Q |= BitTest64(Data, 24) << 0;

    return Q & AREA_1X1_MASK;
}

static uint16_t extract_q1(uint64_t Data)
{
    uint16_t Q = 0U;

    Q |= BitTest64(Data, 0)  << 0;
    Q |= BitTest64(Data, 1)  << 1;
    Q |= BitTest64(Data, 2)  << 2;
    Q |= BitTest64(Data, 3)  << 3;
    Q |= BitTest64(Data, 7)  << 4;
    Q |= BitTest64(Data, 8)  << 5;
    Q |= BitTest64(Data, 9)  << 6;
    Q |= BitTest64(Data, 10) << 7;
    Q |= BitTest64(Data, 14) << 8;
    Q |= BitTest64(Data, 15) << 9;
    Q |= BitTest64(Data, 16) << 10;
    Q |= BitTest64(Data, 17) << 11;

    return Q & AREA_3X4_MASK;
}

static uint16_t extract_q2(uint64_t Data)
{
    uint16_t Q = 0U;

    Q |= BitTest64(Data, 6)  << 0;
    Q |= BitTest64(Data, 13) << 1;
    Q |= BitTest64(Data, 20) << 2;
    Q |= BitTest64(Data, 27) << 3;
    Q |= BitTest64(Data, 5)  << 4;
    Q |= BitTest64(Data, 12) << 5;
    Q |= BitTest64(Data, 19) << 6;
    Q |= BitTest64(Data, 26) << 7;
    Q |= BitTest64(Data, 4)  << 8;
    Q |= BitTest64(Data, 11) << 9;
    Q |= BitTest64(Data, 18) << 10;
    Q |= BitTest64(Data, 25) << 11;

    return Q & AREA_3X4_MASK;
}

static uint16_t extract_q3(uint64_t Data)
{
    uint16_t Q = 0U;

    Q |= BitTest64(Data, 48) << 0;
    Q |= BitTest64(Data, 47) << 1;
    Q |= BitTest64(Data, 46) << 2;
    Q |= BitTest64(Data, 45) << 3;
    Q |= BitTest64(Data, 41) << 4;
    Q |= BitTest64(Data, 40) << 5;
    Q |= BitTest64(Data, 39) << 6;
    Q |= BitTest64(Data, 38) << 7;
    Q |= BitTest64(Data, 34) << 8;
    Q |= BitTest64(Data, 33) << 9;
    Q |= BitTest64(Data, 32) << 10;
    Q |= BitTest64(Data, 31) << 11;

    return Q & AREA_3X4_MASK;
}

static uint16_t extract_q4(uint64_t Data)
{
    uint16_t Q = 0U;

    Q |= BitTest64(Data, 42) << 0;
    Q |= BitTest64(Data, 35) << 1;
    Q |= BitTest64(Data, 28) << 2;
    Q |= BitTest64(Data, 21) << 3;
    Q |= BitTest64(Data, 43) << 4;
    Q |= BitTest64(Data, 36) << 5;
    Q |= BitTest64(Data, 29) << 6;
    Q |= BitTest64(Data, 22) << 7;
    Q |= BitTest64(Data, 44) << 8;
    Q |= BitTest64(Data, 37) << 9;
    Q |= BitTest64(Data, 30) << 10;
    Q |= BitTest64(Data, 23) << 11;

    return Q & AREA_3X4_MASK;
}

static uint64_t convert_q0(uint16_t Data)
{
    uint64_t C = 0ULL;
    
    C |= (uint64_t) BitTest64(Data, 0)  << 24;
    
    return C & BOARD_MASK;
}

static uint64_t convert_q1(uint16_t Data)
{
    uint64_t C = 0ULL;

    C |= (uint64_t) BitTest64(Data, 0)  << 0;
    C |= (uint64_t) BitTest64(Data, 1)  << 1;
    C |= (uint64_t) BitTest64(Data, 2)  << 2;
    C |= (uint64_t) BitTest64(Data, 3)  << 3;
    C |= (uint64_t) BitTest64(Data, 4)  << 7;
    C |= (uint64_t) BitTest64(Data, 5)  << 8;
    C |= (uint64_t) BitTest64(Data, 6)  << 9;
    C |= (uint64_t) BitTest64(Data, 7)  << 10;
    C |= (uint64_t) BitTest64(Data, 8)  << 14;
    C |= (uint64_t) BitTest64(Data, 9)  << 15;
    C |= (uint64_t) BitTest64(Data, 10) << 16;
    C |= (uint64_t) BitTest64(Data, 11) << 17;

    return C & BOARD_MASK;
}

static uint64_t convert_q2(uint16_t Data)
{
    uint64_t C = 0ULL;

    C |= (uint64_t) BitTest64(Data, 0)  << 6;
    C |= (uint64_t) BitTest64(Data, 1)  << 13;
    C |= (uint64_t) BitTest64(Data, 2)  << 20;
    C |= (uint64_t) BitTest64(Data, 3)  << 27;
    C |= (uint64_t) BitTest64(Data, 4)  << 5;
    C |= (uint64_t) BitTest64(Data, 5)  << 12;
    C |= (uint64_t) BitTest64(Data, 6)  << 19;
    C |= (uint64_t) BitTest64(Data, 7)  << 26;
    C |= (uint64_t) BitTest64(Data, 8)  << 4;
    C |= (uint64_t) BitTest64(Data, 9)  << 11;
    C |= (uint64_t) BitTest64(Data, 10) << 18;
    C |= (uint64_t) BitTest64(Data, 11) << 25;

    return C & BOARD_MASK;
}

static uint64_t convert_q3(uint16_t Data)
{
    uint64_t C = 0ULL;

    C |= (uint64_t) BitTest64(Data, 0)  << 48;
    C |= (uint64_t) BitTest64(Data, 1)  << 47;
    C |= (uint64_t) BitTest64(Data, 2)  << 46;
    C |= (uint64_t) BitTest64(Data, 3)  << 45;
    C |= (uint64_t) BitTest64(Data, 4)  << 41;
    C |= (uint64_t) BitTest64(Data, 5)  << 40;
    C |= (uint64_t) BitTest64(Data, 6)  << 39;
    C |= (uint64_t) BitTest64(Data, 7)  << 38;
    C |= (uint64_t) BitTest64(Data, 8)  << 34;
    C |= (uint64_t) BitTest64(Data, 9)  << 33;
    C |= (uint64_t) BitTest64(Data, 10) << 32;
    C |= (uint64_t) BitTest64(Data, 11) << 31;

    return C & BOARD_MASK;
}

static uint64_t convert_q4(uint16_t Data)
{
    uint64_t C = 0ULL;

    C |= (uint64_t) BitTest64(Data, 0)  << 42;
    C |= (uint64_t) BitTest64(Data, 1)  << 35;
    C |= (uint64_t) BitTest64(Data, 2)  << 28;
    C |= (uint64_t) BitTest64(Data, 3)  << 21;
    C |= (uint64_t) BitTest64(Data, 4)  << 43;
    C |= (uint64_t) BitTest64(Data, 5)  << 36;
    C |= (uint64_t) BitTest64(Data, 6)  << 29;
    C |= (uint64_t) BitTest64(Data, 7)  << 22;
    C |= (uint64_t) BitTest64(Data, 8)  << 44;
    C |= (uint64_t) BitTest64(Data, 9)  << 37;
    C |= (uint64_t) BitTest64(Data, 10) << 30;
    C |= (uint64_t) BitTest64(Data, 11) << 23;

    return C & BOARD_MASK;
}

static tIndex board_area_exit_index_q0(tIndex Index)
{
    return BoardAreaExitIndicesQ0[Index];
}

static tIndex board_area_exit_index_q1(tIndex Index)
{
    return BoardAreaExitIndicesQ1[Index];
}

static tIndex board_area_exit_index_q2(tIndex Index)
{
    return BoardAreaExitIndicesQ2[Index];
}

static tIndex board_area_exit_index_q3(tIndex Index)
{
    return BoardAreaExitIndicesQ3[Index];
}

static tIndex board_area_exit_index_q4(tIndex Index)
{
    return BoardAreaExitIndicesQ4[Index];
}
