#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "bitutil.h"
#include "board.h"
#include "debug.h"
#include "scorer.h"
#include "types.h"
#include "util.h"
#include "vector.h"

#define AREA_1X1_EXITS      4
#define AREA_1X1_INDICES    1
#define AREA_1X1_MASK       0x0001U

#define AREA_3X4_INDICES        12
#define AREA_3X4_EXITS          7
#define AREA_3X4_MASK           0x0FFFU
#define AREA_3X4_LOOKUP_SIZE    4096

#define AREA_3x4_MASK_00_02     0x0007U
#define AREA_3x4_MASK_03_05     0x0038U
#define AREA_3x4_MASK_06_08     0x01C0U
#define AREA_3x4_MASK_09_11     0x0E00U

#define AREA_3x4_MASK_00_03     0x000FU
#define AREA_3x4_MASK_04_07     0x00F0U
#define AREA_3x4_MASK_08_11     0x0F00U

#define BOARD_QUADRANTS     4
#define BOARD_Q1_MASK       0x000000000003C78FULL
#define BOARD_Q2_MASK       0x000000000E1C3870ULL
#define BOARD_Q3_MASK       0x0001E3C780000000ULL
#define BOARD_Q4_MASK       0x00001C3870E00000ULL

typedef struct Area3x4Path
{
    uint16_t Path;
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
    uint16_t Area;
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
static uint64_t Area3x4ExpansionLookup[BOARD_QUADRANTS][AREA_3X4_LOOKUP_SIZE] = { 0 };
static uint16_t Area3x4RotationLookup[BOARD_QUADRANTS][AREA_3X4_LOOKUP_SIZE] = { 0 };

static const tIndex Area3x4IndexLookup[ROWS*COLUMNS] = {
    0,  1,  2,  3,  8,  4,  0,
    4,  5,  6,  7,  9,  5,  1,
    8,  9, 10, 11, 10,  6,  2,
    3,  7, 11,  0, 11,  7,  3,
    2,  6, 10, 11, 10,  9,  8,
    1,  5,  9,  7,  6,  5,  4,
    0,  4,  8,  3,  2,  1,  0,
};

static const tIndex Area3x4ExitIndexLookup[AREA_3X4_EXITS] = { 3, 7, 11, 8, 9, 10, 11 };

static const tIndex Area3x4QuadrantLookup[ROWS*COLUMNS] = {
    0, 0, 0, 0, 1, 1, 1,
    0, 0, 0, 0, 1, 1, 1,
    0, 0, 0, 0, 1, 1, 1, 
    3, 3, 3, 0, 1, 1, 1,
    3, 3, 3, 2, 2, 2, 2,
    3, 3, 3, 2, 2, 2, 2,
    3, 3, 3, 2, 2, 2, 2,
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

#define AREA_3X4_INDEX(i)           (Area3x4IndexLookup[i])
#define AREA_3X4_EXIT_INDEX(i)      (Area3x4ExitIndexLookup[i])
#define AREA_3X4_QUADRANT(i)        (Area3x4QuadrantLookup[i])

#define LEFT_VALID(i)       (Area3x4AdjacentIndexLookup[i].LeftValid)
#define RIGHT_VALID(i)      (Area3x4AdjacentIndexLookup[i].RightValid)
#define TOP_VALID(i)        (Area3x4AdjacentIndexLookup[i].TopValid)
#define BOTTOM_VALID(i)     (Area3x4AdjacentIndexLookup[i].BottomValid)

#define LEFT(i)         (Area3x4AdjacentIndexLookup[i].Left)
#define RIGHT(i)        (Area3x4AdjacentIndexLookup[i].Right)
#define TOP(i)          (Area3x4AdjacentIndexLookup[i].Top)
#define BOTTOM(i)       (Area3x4AdjacentIndexLookup[i].Bottom)

static tSize area_3x4_lookup_longest_path(uint64_t Data, tIndex Index, uint64_t *pArea);
static tSize area_1x1_lookup_longest_path(uint64_t Data, tIndex Index, uint64_t *pArea);

static void area_3x4_get_paths(uint16_t Data, tIndex Start, tIndex End, uint16_t Path, tVector *pVector);
static void area_3x4_filter_paths(uint16_t Data, uint16_t MaxPath, tVector *pPaths);

static bool area_3x4_longest_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath);
static tSize area_3x4_index_longest_path(uint16_t Data, tIndex Index);
static bool area_3x4_path_viable(uint16_t Data, uint16_t PrimaryPath, uint16_t MaxPath);
static bool area_3x4_has_path(uint16_t Data, tIndex Start, tIndex End);
static uint16_t area_3x4_area(uint16_t Data, tIndex Index);
static bool area_3x4_exit_adjacent(tIndex Start, tIndex End);
static tSize area_3x4_exits_open(uint16_t Data, uint16_t Path);
static tSize area_3x4_longest_exit_path(uint16_t Data);
static bool area_3x4_subpath(uint16_t Data, uint16_t Path);
static bool area_3x4_subpath_has_longer_through_path(uint16_t Area, uint16_t Path, uint16_t SubPath);

static tIndex board_area_exit_index_q0(tIndex Index);
static tIndex board_area_exit_index_q1(tIndex Index);
static tIndex board_area_exit_index_q2(tIndex Index);
static tIndex board_area_exit_index_q3(tIndex Index);
static tIndex board_area_exit_index_q4(tIndex Index);

static uint64_t area_3x4_expand_q1(uint16_t Data);
static uint64_t area_3x4_expand_q2(uint16_t Data);
static uint64_t area_3x4_expand_q3(uint16_t Data);
static uint64_t area_3x4_expand_q4(uint16_t Data);

static uint16_t area_3x4_contract_q1(uint64_t Data);
static uint16_t area_3x4_contract_q2(uint64_t Data);
static uint16_t area_3x4_contract_q3(uint64_t Data);
static uint16_t area_3x4_contract_q4(uint64_t Data);

static const tSize (*board_area_lookup_longest_path[ROWS*COLUMNS])(uint64_t, tIndex, uint64_t *) = {
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_1x1_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
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

static const uint16_t (*area_3x4_contract_q[ROWS*COLUMNS])(uint64_t) = {
    area_3x4_contract_q1, area_3x4_contract_q1, area_3x4_contract_q1, area_3x4_contract_q1, area_3x4_contract_q2, area_3x4_contract_q2, area_3x4_contract_q2,
    area_3x4_contract_q1, area_3x4_contract_q1, area_3x4_contract_q1, area_3x4_contract_q1, area_3x4_contract_q2, area_3x4_contract_q2, area_3x4_contract_q2,
    area_3x4_contract_q1, area_3x4_contract_q1, area_3x4_contract_q1, area_3x4_contract_q1, area_3x4_contract_q2, area_3x4_contract_q2, area_3x4_contract_q2,
    area_3x4_contract_q4, area_3x4_contract_q4, area_3x4_contract_q4,                 NULL, area_3x4_contract_q2, area_3x4_contract_q2, area_3x4_contract_q2,
    area_3x4_contract_q4, area_3x4_contract_q4, area_3x4_contract_q4, area_3x4_contract_q3, area_3x4_contract_q3, area_3x4_contract_q3, area_3x4_contract_q3,
    area_3x4_contract_q4, area_3x4_contract_q4, area_3x4_contract_q4, area_3x4_contract_q3, area_3x4_contract_q3, area_3x4_contract_q3, area_3x4_contract_q3,
    area_3x4_contract_q4, area_3x4_contract_q4, area_3x4_contract_q4, area_3x4_contract_q3, area_3x4_contract_q3, area_3x4_contract_q3, area_3x4_contract_q3,
};

#define BOARD_AREA_LONGEST_PATH(Data, Index, pArea)     ((*board_area_lookup_longest_path[Index])(Data, Index, pArea))
#define BOARD_AREA_EXIT_INDEX_Q(Index, Exit)            ((*board_area_exit_index_q[Index])(Exit))

#define AREA_3X4_EXPAND_Q(Data, Index)      (Area3x4ExpansionLookup[AREA_3X4_QUADRANT(Index)][Data])
#define AREA_3X4_ROTATE_Q(Data, Index)      (Area3x4RotationLookup[AREA_3X4_QUADRANT(Index)][(*area_3x4_contract_q[Index])(Data)])

void scorer_init()
{
    dbg_printf(DEBUG_LEVEL_INFO, "Initializing scorer lookup");

    int TotalPaths = 0;

    for (uint16_t Data = 0U; Data < AREA_3X4_LOOKUP_SIZE; ++Data)
    {
        tArea3x4Lookup *pLookup = &Area3x4Lookup[Data];

        for (tIndex Start = 0; Start < AREA_3X4_INDICES; ++Start)
        {
            tArea3x4IndexLookup *pIndexLookup = &pLookup->Indices[Start];
                
            if (BitTest16(Data, Start))
            {
                uint16_t Area = area_3x4_area(Data, Start);

                vector_init(&pIndexLookup->Exits);

                pIndexLookup->LongestPath = area_3x4_index_longest_path(Area, Start);
                pIndexLookup->Area = Area;

                for (tIndex Exit = 0; Exit < AREA_3X4_EXITS; ++Exit)
                {
                    tIndex End = AREA_3X4_EXIT_INDEX(Exit);
                    uint16_t MaxPath = 0U;
                    tSize MaxLength;

                    if (BitTest16(Area, End) AND area_3x4_longest_path(Area, Start, End, &MaxLength, &MaxPath))
                    {
                        tArea3x4PathLookup *pPathLookup = emalloc(sizeof(tArea3x4PathLookup));
                        tVector *pPaths = &pPathLookup->Paths;

                        vector_init(pPaths);
                        area_3x4_get_paths(Area, Start, End, 0U, pPaths);
                        area_3x4_filter_paths(Area, MaxPath, pPaths);

                        pPathLookup->Exit = Exit;

                        vector_add(&pIndexLookup->Exits, pPathLookup);

                        TotalPaths += vector_size(pPaths);
                    }
                }
            }
        }

        uint64_t Expansion1 = area_3x4_expand_q1(Data);
        uint64_t Expansion2 = area_3x4_expand_q2(Data);
        uint64_t Expansion3 = area_3x4_expand_q3(Data);
        uint64_t Expansion4 = area_3x4_expand_q4(Data);

        uint16_t Contraction1 = area_3x4_contract_q1(Expansion1);
        uint16_t Contraction2 = area_3x4_contract_q2(Expansion2);
        uint16_t Contraction3 = area_3x4_contract_q3(Expansion3);
        uint16_t Contraction4 = area_3x4_contract_q4(Expansion4);

        Area3x4ExpansionLookup[0][Data] = Expansion1;
        Area3x4ExpansionLookup[1][Data] = Expansion2;
        Area3x4ExpansionLookup[2][Data] = Expansion3;
        Area3x4ExpansionLookup[3][Data] = Expansion4;

        Area3x4RotationLookup[0][Contraction1] = Data;
        Area3x4RotationLookup[1][Contraction2] = Data;
        Area3x4RotationLookup[2][Contraction3] = Data;
        Area3x4RotationLookup[3][Contraction4] = Data;
    }

    dbg_printf(DEBUG_LEVEL_INFO, "Initialized scorer lookup with %d paths", TotalPaths);
}

void scorer_free()
{
    dbg_printf(DEBUG_LEVEL_INFO, "Disposing scorer lookup");

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
                tArea3x4PathLookup *pPaths = vector_iterator_next(&ExitsIterator);
                tVectorIterator PathsIterator;

                vector_iterator_init(&PathsIterator, &pPaths->Paths);

                while (vector_iterator_has_next(&PathsIterator))
                {
                    tArea3x4Path *pPath = vector_iterator_next(&PathsIterator);
                    free(pPath);
                }

                vector_free(&pPaths->Paths);
                free(pPaths);
            }

            vector_free(&pIndexLookup->Exits);
        }
    }

    dbg_printf(DEBUG_LEVEL_INFO, "Disposed scorer lookup");
}

tSize scorer_longest_path(uint64_t Data, tIndex Index, uint64_t *pArea)
{
    return BOARD_AREA_LONGEST_PATH(Data, Index, pArea);
}

static tSize area_3x4_lookup_longest_path(uint64_t Data, tIndex Index, uint64_t *pArea)
{
    tArea3x4Lookup *pLookup = &Area3x4Lookup[AREA_3X4_ROTATE_Q(Data, Index)];
    tArea3x4IndexLookup *pIndexLookup = &pLookup->Indices[AREA_3X4_INDEX(Index)];
    tSize MaxPathLength = 0;

    *pArea |= AREA_3X4_EXPAND_Q(pIndexLookup->Area, Index);

    SET_IF_GREATER(pIndexLookup->LongestPath, MaxPathLength);

    tVector *pExitLookup = &pIndexLookup->Exits;
    tVectorIterator ExitsIterator;

    vector_iterator_init(&ExitsIterator, pExitLookup);

    while (vector_iterator_has_next(&ExitsIterator))
    {
        tArea3x4PathLookup *pPathLookup = vector_iterator_next(&ExitsIterator);
        tIndex NextIndex = BOARD_AREA_EXIT_INDEX_Q(Index, pPathLookup->Exit);

        if (BitTest64(Data, NextIndex))
        {
            tVector *pPaths = &pPathLookup->Paths;
            tVectorIterator PathsIterator;

            vector_iterator_init(&PathsIterator, pPaths);

            while (vector_iterator_has_next(&PathsIterator))
            {
                tArea3x4Path *pPath = vector_iterator_next(&PathsIterator);
                uint64_t PathExpanded = AREA_3X4_EXPAND_Q(pPath->Path, Index);
                tSize LookupLength = pPath->Length + BOARD_AREA_LONGEST_PATH(Data & ~PathExpanded, NextIndex, pArea);

                SET_IF_GREATER(LookupLength, MaxPathLength);
            }
        }
    }

    return MaxPathLength;
}

static tSize area_1x1_lookup_longest_path(uint64_t Data, tIndex Index, uint64_t *pArea)
{
    tSize MaxPathLength = 0;

    BitReset64(&Data, Index);
    BitSet64(pArea, Index);

    for (tIndex Exit = 0; Exit < AREA_1X1_EXITS; ++Exit)
    {
        tIndex NextIndex = BOARD_AREA_EXIT_INDEX_Q(Index, Exit);

        if (BitTest64(Data, NextIndex))
        {
            tSize PathLength = BOARD_AREA_LONGEST_PATH(Data, NextIndex, pArea);
            SET_IF_GREATER(PathLength, MaxPathLength);
        }
    }

    return MaxPathLength + 1;
}

void area_3x4_get_paths(uint16_t Data, tIndex Start, tIndex End, uint16_t Path, tVector *pPaths)
{
    BitSet16(&Path, Start);
    BitReset16(&Data, Start);

    if (Start == End)
    {
        tArea3x4Path *pPath = emalloc(sizeof(tArea3x4Path));

        pPath->Path = Path;
        pPath->Length = BitPopCount16(Path);

        vector_add(pPaths, pPath);

        return;
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start))) area_3x4_get_paths(Data, LEFT(Start), End, Path, pPaths);
    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start))) area_3x4_get_paths(Data, RIGHT(Start), End, Path, pPaths);
    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start))) area_3x4_get_paths(Data, TOP(Start), End, Path, pPaths);
    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start))) area_3x4_get_paths(Data, BOTTOM(Start), End, Path, pPaths);
}

static void area_3x4_filter_paths(uint16_t Area, uint16_t MaxPath, tVector *pPaths)
{
    for (tIndex i = vector_size(pPaths); i > 0; --i)
    {
        tArea3x4Path *pPath = vector_get(pPaths, i - 1);

        if (NOT area_3x4_path_viable(Area, pPath->Path, MaxPath))
        {
            vector_take(pPaths, i - 1);
            free(pPath);
        }
    }

    tVectorIterator Iterator;
    bool DuplicatePath = false;

    vector_iterator_init(&Iterator, pPaths);

    while (vector_iterator_has_next(&Iterator))
    {
        tArea3x4Path *pPath = vector_iterator_next(&Iterator);

        if (pPath->Path == MaxPath)
        {
            DuplicatePath = true;
            break;
        }
    }

    if (NOT DuplicatePath)
    {
        tArea3x4Path *pMaxPath = emalloc(sizeof(tArea3x4Path));
                            
        pMaxPath->Path = MaxPath;
        pMaxPath->Length = BitPopCount16(MaxPath);

        vector_add(pPaths, pMaxPath);
    }

    tVector PathsByLength[AREA_3X4_INDICES] = { 0 };

    for (tIndex Length = 0; Length < AREA_3X4_INDICES; ++Length)
    {
        vector_init(&PathsByLength[Length]);
    }

    vector_iterator_init(&Iterator, pPaths);

    while (vector_iterator_has_next(&Iterator))
    {
        tArea3x4Path *pPath = vector_iterator_next(&Iterator);
        uint16_t *pElement = emalloc(sizeof(uint16_t));

        *pElement = pPath->Path;

        vector_add(&PathsByLength[pPath->Length - 1], pElement);
    }

    for (tIndex Length = 0; Length < AREA_3X4_INDICES; ++Length)
    {
        tVector *pVector = &PathsByLength[Length];

        if (vector_size(pVector) > 1)
        {
            tVector BestElements;
            tSize MaxExitsOpen = 0;
            tSize MaxSecondaryPathLen = 0;

            vector_init(&BestElements);
            vector_iterator_init(&Iterator, pVector);

            while (vector_iterator_has_next(&Iterator))
            {
                uint16_t *pElement = vector_iterator_next(&Iterator);
                
                tSize ExitsOpen = area_3x4_exits_open(Area, *pElement);
                tSize SecondaryPathLen = area_3x4_longest_exit_path(Area & ~*pElement);

                if (SecondaryPathLen == MaxSecondaryPathLen)
                {
                    if (ExitsOpen == MaxExitsOpen)
                    {
                        vector_add(&BestElements, pElement);
                    }
                    else if (ExitsOpen > MaxExitsOpen)
                    {
                        while (vector_pop(&BestElements) ISNOT NULL);

                        vector_add(&BestElements, pElement);

                        MaxSecondaryPathLen = SecondaryPathLen;
                        MaxExitsOpen = ExitsOpen;
                    }
                }
                else if (SecondaryPathLen > MaxSecondaryPathLen)
                {
                    while (vector_pop(&BestElements) ISNOT NULL);

                    vector_add(&BestElements, pElement);

                    MaxSecondaryPathLen = SecondaryPathLen;
                    MaxExitsOpen = ExitsOpen;
                }
            }

            for (tIndex i = vector_size(pPaths); i > 0; --i)
            {
                tArea3x4Path *pPath = vector_get(pPaths, i - 1);
                bool Contains = false;

                vector_iterator_init(&Iterator, &BestElements);

                while (vector_iterator_has_next(&Iterator))
                {
                    uint16_t *pElement = vector_iterator_next(&Iterator);

                    if (*pElement == pPath->Path)
                    {
                        Contains = true;
                        break;
                    }
                }

                if (pPath->Length == Length + 1 AND NOT Contains)
                {
                    vector_take(pPaths, i - 1);
                    free(pPath);
                }
            }

            vector_free(&BestElements);
        }
    }

    tVector SubPathsToDelete;

    vector_init(&SubPathsToDelete);

    for (tIndex i = 0; i < vector_size(pPaths); ++i)
    {
        tArea3x4Path *pSubPath = vector_get(pPaths, i);
        tSize SubExitCount = area_3x4_exits_open(Area, pSubPath->Path);
        tSize SubPathLen = BitPopCount16(pSubPath->Path);
        tSize SecondarySubPathLen = area_3x4_longest_exit_path(Area & ~pSubPath->Path);

        for (tIndex j = 0; j < vector_size(pPaths); ++j)
        {
            tArea3x4Path *pPath = vector_get(pPaths, j);
            tSize ExitCount = area_3x4_exits_open(Area, pPath->Path);
            tSize PathLen = BitPopCount16(pPath->Path);
            tSize SecondaryPathLen = area_3x4_longest_exit_path(Area & ~pPath->Path);

            if (area_3x4_subpath(pPath->Path, pSubPath->Path)
                AND ExitCount >= SubExitCount
                AND PathLen + SecondaryPathLen >= SubPathLen + SecondarySubPathLen
                AND NOT area_3x4_subpath_has_longer_through_path(Area, pPath->Path, pSubPath->Path))
            {
                vector_add(&SubPathsToDelete, pSubPath);
            }
        }
    }

    tSize MaxPathLength = BitPopCount16(MaxPath);

    vector_iterator_init(&Iterator, pPaths);

    while (vector_iterator_has_next(&Iterator))
    {
        tArea3x4Path *pPath = vector_iterator_next(&Iterator);

        if (pPath->Length < MaxPathLength - 8)
        {
            vector_add(&SubPathsToDelete, pPath);
        }
    }

    vector_iterator_init(&Iterator, &SubPathsToDelete);

    while (vector_iterator_has_next(&Iterator))
    {
        tArea3x4Path *pSubPath = vector_iterator_next(&Iterator);

        for (tSize i = vector_size(pPaths); i > 0; --i)
        {
            tArea3x4Path *pPath = vector_get(pPaths, i - 1);

            if (pPath->Path == pSubPath->Path)
            {
                vector_take(pPaths, i - 1);
                free(pPath);
            }
        }
    }

    vector_free(&SubPathsToDelete);

    for (tIndex Length = 0; Length < AREA_3X4_INDICES; ++Length)
    {
        tVector *pVector = &PathsByLength[Length];

        vector_iterator_init(&Iterator, pVector);

        while (vector_iterator_has_next(&Iterator))
        {
            free(vector_iterator_next(&Iterator));
        }

        vector_free(pVector);
    }
}

bool area_3x4_longest_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath)
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

tSize area_3x4_index_longest_path(uint16_t Data, tIndex Index)
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

bool area_3x4_path_viable(uint16_t Data, uint16_t PrimaryPath, uint16_t MaxPath)
{
    bool PathFound = false;
    uint16_t Remain = Data & ~PrimaryPath;

    for (tIndex StartExit = 0; StartExit < AREA_3X4_EXITS; ++StartExit)
    {
        for (tIndex EndExit = 0; EndExit < AREA_3X4_EXITS; ++EndExit)
        {
            if (StartExit != EndExit)
            {
                tIndex Start = AREA_3X4_EXIT_INDEX(StartExit);
                tIndex End = AREA_3X4_EXIT_INDEX(EndExit);

                if (BitTest16(Remain, Start)
                    AND BitTest16(Remain, End)
                    AND NOT area_3x4_exit_adjacent(StartExit, EndExit)
                    AND area_3x4_has_path(Remain, Start, End))
                {
                    PathFound = true;
                    goto Done;
                }
            }
        }
    }

    tSize PrimaryPathLength = BitPopCount16(PrimaryPath);
    tSize MaxLength = BitPopCount16(MaxPath);
    tSize MaxPathExitsOpen = area_3x4_exits_open(Data, MaxPath);

    for (tIndex StartExit = 0; StartExit < AREA_3X4_EXITS; ++StartExit)
    {
        for (tIndex End = 0; End < AREA_3X4_INDICES; ++End)
        {
            tIndex Start = AREA_3X4_EXIT_INDEX(StartExit);
            tSize Length = 0U;
            uint16_t SecondaryPath = 0U;

            if (BitTest16(Remain, Start)
                AND BitTest16(Remain, End)
                AND area_3x4_longest_path(Remain, Start, End, &Length, &SecondaryPath)
                AND area_3x4_exits_open(Data, PrimaryPath) >= MaxPathExitsOpen
                AND PrimaryPathLength + Length > MaxLength)
            {
                PathFound = true;
                goto Done;
            }
        }
    }

Done:
    return PathFound;
}

bool area_3x4_has_path(uint16_t Data, tIndex Start, tIndex End)
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

uint16_t area_3x4_area(uint16_t Data, tIndex Index)
{
    uint16_t Area = 0U;

    BitSet16(&Area, Index);
    BitReset16(&Data, Index);

    if (LEFT_VALID(Index) AND BitTest16(Data, LEFT(Index))) Area |= area_3x4_area(Data, LEFT(Index));
    if (RIGHT_VALID(Index) AND BitTest16(Data, RIGHT(Index))) Area |= area_3x4_area(Data, RIGHT(Index));
    if (TOP_VALID(Index) AND BitTest16(Data, TOP(Index))) Area |= area_3x4_area(Data, TOP(Index));
    if (BOTTOM_VALID(Index) AND BitTest16(Data, BOTTOM(Index))) Area |= area_3x4_area(Data, BOTTOM(Index));

    return Area;
}

static bool area_3x4_exit_adjacent(tIndex Start, tIndex End)
{
    return ((Start <= 2 AND End <= 2) OR (Start >= 3 AND End >= 3));
}

static tSize area_3x4_exits_open(uint16_t Data, uint16_t Path)
{
    tSize Count = 0;

    for (tIndex Exit = 0; Exit < AREA_3X4_EXITS; ++Exit)
    {
        tIndex Index = Area3x4ExitIndexLookup[Exit];

        if (BitTest16(Data, Index) AND NOT BitTest16(Path, Index))
        {
            Count++;
        }
    }

    return Count;
}

static tSize area_3x4_longest_exit_path(uint16_t Data)
{
    tSize PathLength, MaxPathLength = 0;

    for (tIndex Exit = 0; Exit < AREA_3X4_EXITS; ++Exit)
    {
        tIndex Index = AREA_3X4_EXIT_INDEX(Exit);

        if (BitTest16(Data, Index))
        {
            PathLength = area_3x4_index_longest_path(Data, Index);
            SET_IF_GREATER(PathLength, MaxPathLength);
        }
    }

    return MaxPathLength;
}

static bool area_3x4_subpath(uint16_t Data, uint16_t Path)
{
    return BitPopCount16(Data) > BitPopCount16(Path) AND BitEmpty16(Path & ~Data);
}

static bool area_3x4_subpath_has_longer_through_path(uint16_t Area, uint16_t Path, uint16_t SubPath)
{
    bool HasLonger = false;

    uint16_t SecondaryArea = Area & ~Path, SecondarySubArea = Area & ~SubPath;

    for (tIndex StartExit = 0; StartExit < AREA_3X4_EXITS; ++StartExit)
    {
        for (tIndex EndExit = 0; EndExit < AREA_3X4_EXITS; ++EndExit)
        {
            if (StartExit != EndExit)
            {
                tIndex Start = AREA_3X4_EXIT_INDEX(StartExit), End = AREA_3X4_EXIT_INDEX(EndExit);

                uint16_t SecondaryPath = 0U, SubSecondaryPath = 0U;
                tSize SecondaryPathLength = 0U, SubSecondaryPathLength = 0U;
                
                bool SecondaryHas = area_3x4_longest_path(SecondaryArea, Start, End, &SecondaryPathLength, &SecondaryPath);
                bool SubSecondaryHas = area_3x4_longest_path(SecondarySubArea, Start, End, &SubSecondaryPathLength, &SubSecondaryPath);

                if (SecondaryHas AND SubSecondaryHas AND SubSecondaryPathLength > SecondaryPathLength)
                {
                    HasLonger = true;
                    break;
                }
            }
        }
    }

    return HasLonger;
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

static uint64_t area_3x4_expand_q1(uint16_t Data)
{
    uint64_t E = 0ULL;

    E |= (uint64_t) BitTest64(Data, 0)  << 0;
    E |= (uint64_t) BitTest64(Data, 1)  << 1;
    E |= (uint64_t) BitTest64(Data, 2)  << 2;
    E |= (uint64_t) BitTest64(Data, 3)  << 3;
    E |= (uint64_t) BitTest64(Data, 4)  << 7;
    E |= (uint64_t) BitTest64(Data, 5)  << 8;
    E |= (uint64_t) BitTest64(Data, 6)  << 9;
    E |= (uint64_t) BitTest64(Data, 7)  << 10;
    E |= (uint64_t) BitTest64(Data, 8)  << 14;
    E |= (uint64_t) BitTest64(Data, 9)  << 15;
    E |= (uint64_t) BitTest64(Data, 10) << 16;
    E |= (uint64_t) BitTest64(Data, 11) << 17;

    return E & BOARD_MASK;
}

static uint64_t area_3x4_expand_q2(uint16_t Data)
{
    uint64_t E = 0ULL;

    E |= (uint64_t) BitTest64(Data, 0)  << 6;
    E |= (uint64_t) BitTest64(Data, 1)  << 13;
    E |= (uint64_t) BitTest64(Data, 2)  << 20;
    E |= (uint64_t) BitTest64(Data, 3)  << 27;
    E |= (uint64_t) BitTest64(Data, 4)  << 5;
    E |= (uint64_t) BitTest64(Data, 5)  << 12;
    E |= (uint64_t) BitTest64(Data, 6)  << 19;
    E |= (uint64_t) BitTest64(Data, 7)  << 26;
    E |= (uint64_t) BitTest64(Data, 8)  << 4;
    E |= (uint64_t) BitTest64(Data, 9)  << 11;
    E |= (uint64_t) BitTest64(Data, 10) << 18;
    E |= (uint64_t) BitTest64(Data, 11) << 25;

    return E & BOARD_MASK;
}

static uint64_t area_3x4_expand_q3(uint16_t Data)
{
    uint64_t E = 0ULL;

    E |= (uint64_t) BitTest64(Data, 0)  << 48;
    E |= (uint64_t) BitTest64(Data, 1)  << 47;
    E |= (uint64_t) BitTest64(Data, 2)  << 46;
    E |= (uint64_t) BitTest64(Data, 3)  << 45;
    E |= (uint64_t) BitTest64(Data, 4)  << 41;
    E |= (uint64_t) BitTest64(Data, 5)  << 40;
    E |= (uint64_t) BitTest64(Data, 6)  << 39;
    E |= (uint64_t) BitTest64(Data, 7)  << 38;
    E |= (uint64_t) BitTest64(Data, 8)  << 34;
    E |= (uint64_t) BitTest64(Data, 9)  << 33;
    E |= (uint64_t) BitTest64(Data, 10) << 32;
    E |= (uint64_t) BitTest64(Data, 11) << 31;

    return E & BOARD_MASK;
}

static uint64_t area_3x4_expand_q4(uint16_t Data)
{
    uint64_t E = 0ULL;

    E |= (uint64_t) BitTest64(Data, 0)  << 42;
    E |= (uint64_t) BitTest64(Data, 1)  << 35;
    E |= (uint64_t) BitTest64(Data, 2)  << 28;
    E |= (uint64_t) BitTest64(Data, 3)  << 21;
    E |= (uint64_t) BitTest64(Data, 4)  << 43;
    E |= (uint64_t) BitTest64(Data, 5)  << 36;
    E |= (uint64_t) BitTest64(Data, 6)  << 29;
    E |= (uint64_t) BitTest64(Data, 7)  << 22;
    E |= (uint64_t) BitTest64(Data, 8)  << 44;
    E |= (uint64_t) BitTest64(Data, 9)  << 37;
    E |= (uint64_t) BitTest64(Data, 10) << 30;
    E |= (uint64_t) BitTest64(Data, 11) << 23;

    return E & BOARD_MASK;
}

static uint16_t area_3x4_contract_q1(uint64_t Data)
{
    uint64_t C = Data & BOARD_Q1_MASK;

    return (C >> 6 & AREA_3x4_MASK_08_11) 
         | (C >> 3 & AREA_3x4_MASK_04_07)
         | (C >> 0 & AREA_3x4_MASK_00_03);
}

static uint16_t area_3x4_contract_q2(uint64_t Data)
{
    uint64_t C = Data & BOARD_Q2_MASK;

    return (C >> 16 & AREA_3x4_MASK_09_11)
         | (C >> 12 & AREA_3x4_MASK_06_08)
         | (C >> 8  & AREA_3x4_MASK_03_05)
         | (C >> 4  & AREA_3x4_MASK_00_02);
}

static uint16_t area_3x4_contract_q3(uint64_t Data)
{
    uint64_t C = Data & BOARD_Q3_MASK;

    return (C >> 37 & AREA_3x4_MASK_08_11)
         | (C >> 34 & AREA_3x4_MASK_04_07)
         | (C >> 31 & AREA_3x4_MASK_00_03);
}

static uint16_t area_3x4_contract_q4(uint64_t Data)
{
    uint64_t C = Data & BOARD_Q4_MASK;

    return (C >> 33 & AREA_3x4_MASK_09_11)
         | (C >> 29 & AREA_3x4_MASK_06_08)
         | (C >> 25 & AREA_3x4_MASK_03_05)
         | (C >> 21 & AREA_3x4_MASK_00_02);
}
