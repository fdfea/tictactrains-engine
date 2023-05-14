#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "bitutil.h"
#include "board.h"
#include "scorer.h"
#include "types.h"
#include "util.h"
#include "vector.h"

#include <stdio.h>
#include <string.h>

static void area_3x4_print(uint16_t Data);
static void area_7x7_print(uint64_t Data);

int TotalPaths = 0;

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
    uint64_t Path; //might be able to change this to uint16_t
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

//static tSize area_3x4_index_longest_path(uint16_t Data, tIndex Index);
//static bool area_3x4_longest_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath);
//static bool area_3x4_has_through_path(uint16_t Data);
//static bool area_3x4_has_path(uint16_t Data, tIndex Start, tIndex End);
//static void area_3x4_get_lookup_paths(uint16_t Data, tIndex Start, tIndex End, uint16_t Path, tVector *pVector);

static tSize area_3x4_lookup_longest_path(uint64_t Data, tIndex Index);
static tSize area_1x1_lookup_longest_path(uint64_t Data, tIndex Index);

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

static const tSize (*board_area_lookup_longest_path[ROWS*COLUMNS])(uint64_t, tIndex) = {
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

#define BOARD_AREA_LONGEST_PATH(Data, Index)    ((*board_area_lookup_longest_path[Index])(Data, Index))
#define BOARD_AREA_EXIT_INDEX_Q(Index, Exit)    ((*board_area_exit_index_q[Index])(Exit))

#define AREA_3X4_EXPAND_Q(Data, Index)      (Area3x4ExpansionLookup[AREA_3X4_QUADRANT(Index)][Data])
#define AREA_3X4_ROTATE_Q(Data, Index)      (Area3x4RotationLookup[AREA_3X4_QUADRANT(Index)][(*area_3x4_contract_q[Index])(Data)])

static bool area_3x4_exit_adjacent(int Start, int End);
static tSize area_3x4_exits_open(uint16_t Data, uint16_t Path);
static tSize area_3x4_longest_exit_path(uint16_t Data);
static bool area_3x4_subpath(uint16_t Data, uint16_t Path);
static bool area_3x4_subpath_has_longer_through_path(uint16_t Area, uint16_t Path, uint16_t SubPath);

void scorer_init()
{
    for (uint16_t Data = 0U; Data < AREA_3X4_LOOKUP_SIZE; ++Data)
    {
        tArea3x4Lookup *pLookup = &Area3x4Lookup[Data];

        for (tIndex Start = 0; Start < AREA_3X4_INDICES; ++Start)
        {
            tArea3x4IndexLookup *pIndexLookup = &pLookup->Indices[Start];
                
            if (BitTest16(Data, Start))
            {
                uint16_t Area = 0U;

                area_3x4_area(Data, Start, &Area);
                vector_init(&pIndexLookup->Exits);

                pIndexLookup->LongestPath = area_3x4_index_longest_path(Area, Start);

                for (tIndex Exit = 0; Exit < AREA_3X4_EXITS; ++Exit)
                {
                    tIndex End = AREA_3X4_EXIT_INDEX(Exit);
                    uint16_t Path = 0U;
                    tSize Length;

                    // don't need to check if path exists now?
                    if (BitTest16(Area, End) AND area_3x4_longest_path(Area, Start, End, &Length, &Path))
                    {
                        tArea3x4PathLookup *pPathLookup = emalloc(sizeof(tArea3x4PathLookup));

                        vector_init(&pPathLookup->Paths);
                        area_3x4_get_lookup_paths(Area, Start, End, 0U, &pPathLookup->Paths, Path);

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

                        /*
                        if (Area == 0b111011000100 AND Start == 9 AND End == 7 AND Exit == 1)
                        {
                            printf("=========== start: %d, exit: %d, end: %d, max length: %d ===========\n", Start, Exit, End, Length);
                            printf("DATA\n");
                            area_3x4_print(Data);
                            //printf("USED\n");
                            //area_3x4_print(Used);
                            printf("FOUND PATHS: %d\n", vector_size(&pPathLookup->Paths));
                            vector_iterator_init(&PathsIterator, &pPathLookup->Paths);
                            while (vector_iterator_has_next(&PathsIterator))
                            {
                                tArea3x4Path *pPath = vector_iterator_next(&PathsIterator);
                                if (NOT BitEmpty16(pPath->Path & ~Data))
                                {
                                    printf("bad path!!!\n");
                                }
                                printf("length: %d, path:\n", pPath->Length);
                                area_3x4_print(pPath->Path);
                            }
                        }
                        */

                        // start crazy stuff

                        // for all paths with same length, only keep one with most open exits and best 2nd length
                        typedef struct PathElement
                        {
                            uint16_t Path;
                        }
                        tPathElement;

                        tVector PathLengthsTable[13] = { 0 };

                        for (int i = 0; i < 13; ++i)
                        {
                            vector_init(&PathLengthsTable[i]);
                        }

                        for (tIndex Index = vector_size(&pPathLookup->Paths); Index > 0; --Index)
                        {
                            tArea3x4Path *pPath = vector_get(&pPathLookup->Paths, Index - 1);

                            tPathElement *pElement = emalloc(sizeof(tPathElement));
                            pElement->Path = (uint16_t) pPath->Path;

                            if (pPath->Length > 12)
                            {
                                printf("bad path length\n");
                                exit(1);
                            }

                            vector_add(&PathLengthsTable[pPath->Length], pElement);
                        }

                        /*
                        printf("PATH LENGTHS TABLE\n");
                        for (int i = 0; i < AREA_3X4_INDICES; ++i)
                        {
                            tVector *pVector = &PathLengthsTable[i];

                            if (vector_size(pVector) > 1)
                            {
                                printf("LENGTH: %d\n", i);
                                printf("AREA\n");
                                area_3x4_print(Area);

                                for (int j = 0; j < vector_size(pVector); j++)
                                {
                                    tPathElement *pElement = (tPathElement *) vector_get(pVector, j);
                                    printf("PATH\n");
                                    area_3x4_print(pElement->Path);
                                }
                            }
                        }
                        */

                        for (int i = 0; i < 13; ++i)
                        {
                            tVector *pVector = &PathLengthsTable[i];

                            if (vector_size(pVector) > 1)
                            {
                                //printf("found one greater than 1\n");

                                tVector BestElements;
                                vector_init(&BestElements);

                                int MaxExitsOpen = -1;
                                int MaxSecPathLen = -1;
                                //tPathElement *pBest = NULL;

                                // if amount of exits is the same, may need to check for longest end-path / through-path
                                for (int k = 0; k < vector_size(pVector); ++k)
                                {
                                    //don't want least exit path, but path with maximal secondary path?
                                    tPathElement *pElement = (tPathElement *) vector_get(pVector, k);
                                    int ExitsOpen = area_3x4_exits_open(Area, pElement->Path);
                                    int SecPathLen = area_3x4_longest_exit_path(Area & ~pElement->Path);

                                    if (SecPathLen == MaxSecPathLen)
                                    {
                                        if (ExitsOpen == MaxExitsOpen)
                                        {
                                            vector_add(&BestElements, pElement);
                                        }
                                        // this added 3 paths lol
                                        else if (ExitsOpen > MaxExitsOpen)
                                        {
                                            for (int w = vector_size(&BestElements); w > 0; --w)
                                            {
                                                vector_take(&BestElements, w - 1);
                                            }

                                            vector_add(&BestElements, pElement);
                                            MaxSecPathLen = SecPathLen;
                                            MaxExitsOpen = ExitsOpen;
                                        }
                                    }
                                    else
                                    {
                                        if (SecPathLen > MaxSecPathLen)
                                        {
                                            for (int w = vector_size(&BestElements); w > 0; --w)
                                            {
                                                vector_take(&BestElements, w - 1);
                                            }

                                            vector_add(&BestElements, pElement);
                                            MaxSecPathLen = SecPathLen;
                                            MaxExitsOpen = ExitsOpen;
                                        }
                                    }
                                }

                                if (vector_size(&BestElements) == 0 OR /*MaxExitsOpen*/ MaxSecPathLen < 0)
                                {
                                    printf("something bad happened\n");
                                    exit(1);
                                }

                                for (int j = 0; j < vector_size(pVector); ++j)
                                {
                                    tPathElement *pElement = (tPathElement *) vector_get(pVector, j);

                                    if (BitPopCount16(pElement->Path) != i)
                                    {
                                        printf("bad length\n");
                                        exit(1);
                                    }

                                    bool OneOfBest = false;
                                    for (int g = 0; g < vector_size(&BestElements); ++g)
                                    {
                                        tPathElement *pEE = vector_get(&BestElements, g);

                                        if (pEE->Path == pElement->Path)
                                        {
                                            OneOfBest = true;
                                            break;
                                        }
                                    }

                                    if (NOT OneOfBest)
                                    {
                                        for (int k = vector_size(&pPathLookup->Paths); k > 0; --k)
                                        {
                                            tArea3x4Path *pPath = vector_get(&pPathLookup->Paths, k - 1);

                                            if (pPath->Length == i AND pPath->Path == pElement->Path)
                                            {
                                                //printf("taking stuff\n");
                                                vector_take(&pPathLookup->Paths, k - 1);
                                                free(pPath);
                                                break;
                                            }
                                        }
                                    }
                                }

                                vector_free(&BestElements);
                            }
                        }

                        /*
                        if (Area == 0b111011000100 AND Start == 9 AND End == 7 AND Exit == 1)
                        {
                            printf("=========== start: %d, exit: %d, end: %d, max length: %d ===========\n", Start, Exit, End, Length);
                            printf("DATA\n");
                            area_3x4_print(Data);
                            //printf("USED\n");
                            //area_3x4_print(Used);
                            printf("FOUND PATHS: %d\n", vector_size(&pPathLookup->Paths));
                            vector_iterator_init(&PathsIterator, &pPathLookup->Paths);
                            while (vector_iterator_has_next(&PathsIterator))
                            {
                                tArea3x4Path *pPath = vector_iterator_next(&PathsIterator);
                                if (NOT BitEmpty16(pPath->Path & ~Data))
                                {
                                    printf("bad path!!!\n");
                                }
                                printf("length: %d, path:\n", pPath->Length);
                                area_3x4_print(pPath->Path);
                            }
                        }
                        */

                        // if a shorter path is a subpath of a longer path and longer path has same exit count, don't need shorter path?
                        tVector PathsToDelete;
                        vector_init(&PathsToDelete);

                        // also need to make sure potential through-path length isn't smaller
                        // longer path length >= sub path + longest secondary path
                        for (int c = 0; c < vector_size(&pPathLookup->Paths); ++c)
                        {
                            tArea3x4Path *pSubPath = vector_get(&pPathLookup->Paths, c);
                            tSize SubExitCount = area_3x4_exits_open(Area, pSubPath->Path);
                            tSize SubPathLen = BitPopCount16(pSubPath->Path);
                            tSize SecSubPathLen = area_3x4_longest_exit_path(Area & ~pSubPath->Path);

                            for (int d = 0; d < vector_size(&pPathLookup->Paths); ++d)
                            {
                                tArea3x4Path *pPath = vector_get(&pPathLookup->Paths, d);
                                tSize ExitCount = area_3x4_exits_open(Area, pPath->Path);
                                tSize PathLen = BitPopCount16(pPath->Path);
                                tSize SecPathLen = area_3x4_longest_exit_path(Area & ~pPath->Path);

                                // also need to check all through paths to make sure they don't get smaller?

                                // AND DOES NOT HAVE LONGER THROUGH PATH FOR GIVEN ENTRY/EXIT

                                if (area_3x4_subpath(pPath->Path, pSubPath->Path)
                                    AND ExitCount >= SubExitCount
                                    AND PathLen + SecPathLen >= SubPathLen + SecSubPathLen
                                    AND NOT area_3x4_subpath_has_longer_through_path(Area, pPath->Path, pSubPath->Path))
                                {
                                    //printf("adding path to delete\n");
                                    vector_add(&PathsToDelete, pSubPath);
                                }
                            }
                        }

                        for (int m = 0; m < vector_size(&PathsToDelete); ++m)
                        {
                            tArea3x4Path *pSubPath = vector_get(&PathsToDelete, m);

                            for (int k = vector_size(&pPathLookup->Paths); k > 0; --k)
                            {
                                tArea3x4Path *pPath = vector_get(&pPathLookup->Paths, k - 1);

                                if (pSubPath->Path == pPath->Path)
                                {
                                    vector_take(&pPathLookup->Paths, k - 1);
                                    free(pPath);
                                    break;
                                }
                            }
                        }

                        vector_free(&PathsToDelete);

                        for (int i = 0; i < 13; ++i)
                        {
                            tVector *pVector = &PathLengthsTable[i];

                            for (int k = 0; k < vector_size(pVector); ++k)
                            {
                                tPathElement *pElement = (tPathElement *) vector_get(pVector, k);
                                free(pElement);
                            }

                            vector_free(pVector);
                        }

                        /*
                        if (Area == 0b111011000100 AND Start == 9 AND End == 7 AND Exit == 1)
                        {
                            printf("=========== start: %d, exit: %d, end: %d, max length: %d ===========\n", Start, Exit, End, Length);
                            printf("DATA\n");
                            area_3x4_print(Data);
                            //printf("USED\n");
                            //area_3x4_print(Used);
                            printf("FOUND PATHS: %d\n", vector_size(&pPathLookup->Paths));
                            vector_iterator_init(&PathsIterator, &pPathLookup->Paths);
                            while (vector_iterator_has_next(&PathsIterator))
                            {
                                tArea3x4Path *pPath = vector_iterator_next(&PathsIterator);
                                if (NOT BitEmpty16(pPath->Path & ~Data))
                                {
                                    printf("bad path!!!\n");
                                }
                                printf("length: %d, path:\n", pPath->Length);
                                area_3x4_print(pPath->Path);
                            }
                        }
                        */

                        TotalPaths += vector_size(&pPathLookup->Paths);
                    }
                }
            }
        }

        uint64_t Expansion1 = area_3x4_expand_q1(Data);
        uint64_t Expansion2 = area_3x4_expand_q2(Data);
        uint64_t Expansion3 = area_3x4_expand_q3(Data);
        uint64_t Expansion4 = area_3x4_expand_q4(Data);

        Area3x4ExpansionLookup[0][Data] = Expansion1;
        Area3x4ExpansionLookup[1][Data] = Expansion2;
        Area3x4ExpansionLookup[2][Data] = Expansion3;
        Area3x4ExpansionLookup[3][Data] = Expansion4;

        Area3x4RotationLookup[0][area_3x4_contract_q1(Expansion1)] = Data;
        Area3x4RotationLookup[1][area_3x4_contract_q2(Expansion2)] = Data;
        Area3x4RotationLookup[2][area_3x4_contract_q3(Expansion3)] = Data;
        Area3x4RotationLookup[3][area_3x4_contract_q4(Expansion4)] = Data;
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

    TotalPaths = 0;
}

tScore scorer_score(tBoard *pBoard)
{
    tScore ScoreX = 0, ScoreO = 0;

    for (tIndex Index = 0; Index < ROWS*COLUMNS; ++Index)
    {
        bool Player = board_index_player(pBoard, Index);
        uint64_t Data = IF Player THEN pBoard->Data ELSE ~pBoard->Data & BOARD_MASK;
        tScore Score = BOARD_AREA_LONGEST_PATH(Data, Index);

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

static tSize area_3x4_lookup_longest_path(uint64_t Data, tIndex Index)
{
    tArea3x4Lookup *pLookup = &Area3x4Lookup[AREA_3X4_ROTATE_Q(Data, Index)];
    tArea3x4IndexLookup *pIndexLookup = &pLookup->Indices[AREA_3X4_INDEX(Index)];
    tSize MaxPathLength = 0;

    //make list of tArea3x4Paths in longest path and set Used++ for each?

    SET_IF_GREATER(pIndexLookup->LongestPath, MaxPathLength);

    tVector *pExitLookup = &pIndexLookup->Exits;
    tVectorIterator ExitsIterator;

    vector_iterator_init(&ExitsIterator, pExitLookup);

    while (vector_iterator_has_next(&ExitsIterator))
    {
        tArea3x4PathLookup *pPathLookup = (tArea3x4PathLookup *) vector_iterator_next(&ExitsIterator);
        tIndex NextIndex = BOARD_AREA_EXIT_INDEX_Q(Index, pPathLookup->Exit);

        if (BitTest64(Data, NextIndex))
        {
            tVector *pPaths = &pPathLookup->Paths;
            tVectorIterator PathsIterator;

            vector_iterator_init(&PathsIterator, pPaths);

            while (vector_iterator_has_next(&PathsIterator))
            {
                tArea3x4Path *pPath = (tArea3x4Path *) vector_iterator_next(&PathsIterator);
                uint64_t PathExpanded = AREA_3X4_EXPAND_Q(pPath->Path, Index);
                tSize LookupLength = pPath->Length + BOARD_AREA_LONGEST_PATH(Data & ~PathExpanded, NextIndex);

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

    for (tIndex Exit = 0; Exit < AREA_1X1_EXITS; ++Exit)
    {
        tIndex NextIndex = BOARD_AREA_EXIT_INDEX_Q(Index, Exit);

        if (BitTest64(Data, NextIndex))
        {
            tSize PathLength = BOARD_AREA_LONGEST_PATH(Data, NextIndex);
            SET_IF_GREATER(PathLength, MaxPathLength);
        }
    }

    return MaxPathLength + 1;
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

// you already know a path exists before calling this
// traverses all paths but only keeps those with a "through-path"
void area_3x4_get_lookup_paths(uint16_t Data, tIndex Start, tIndex End, uint16_t Path, tVector *pPaths, uint16_t MaxPath)
{
    BitSet16(&Path, Start);
    BitReset16(&Data, Start);

    if (Start == End AND area_3x4_has_through_path(Data & ~Path, Path, MaxPath))
    {
        tArea3x4Path *pPath = emalloc(sizeof(tArea3x4Path));

        pPath->Path = Path;
        pPath->Length = BitPopCount16(Path);

        vector_add(pPaths, (void *) pPath);

        return;
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start))) area_3x4_get_lookup_paths(Data, LEFT(Start), End, Path, pPaths, MaxPath);
    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start))) area_3x4_get_lookup_paths(Data, RIGHT(Start), End, Path, pPaths, MaxPath);
    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start))) area_3x4_get_lookup_paths(Data, TOP(Start), End, Path, pPaths, MaxPath);
    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start))) area_3x4_get_lookup_paths(Data, BOTTOM(Start), End, Path, pPaths, MaxPath);
}

// pass in start data here and check for blocking through-path?
// probably need to compare to other computed paths at some point
/*
bool area_3x4_has_through_path(uint16_t Data)
{
    bool PathFound = false;

    for (tIndex StartExit = 0; StartExit < AREA_3X4_EXITS; ++StartExit)
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
*/

bool area_3x4_has_through_path(uint16_t Data, uint16_t PrimaryPath, uint16_t MaxPath)
{
    bool PathFound = false;

    // only consider squares that touch original data

    // find true through-paths
    // non-adjacent
    for (tIndex StartExit = 0; StartExit < AREA_3X4_EXITS; ++StartExit)
    {
        for (tIndex EndExit = 0; EndExit < AREA_3X4_EXITS; ++EndExit)
        {
            if (StartExit != EndExit)
            {
                tIndex Start = AREA_3X4_EXIT_INDEX(StartExit), End = AREA_3X4_EXIT_INDEX(EndExit);

                if (BitTest16(Data, Start)
                    AND BitTest16(Data, End)
                    AND NOT area_3x4_exit_adjacent(StartExit, EndExit)
                    AND area_3x4_has_path(Data, Start, End))
                {
                    PathFound = true;
                    goto Done;
                }
            }
        }
    }

    uint16_t Original = Data | PrimaryPath;
    tSize PrimaryPathLength = BitPopCount16(PrimaryPath);
    tSize MaxLength = BitPopCount16(MaxPath);
    tSize MaxPathExitsOpen = area_3x4_exits_open(Original, MaxPath);

    // find viable end-paths
    for (tIndex StartExit = 0; StartExit < AREA_3X4_EXITS; ++StartExit)
    {
        for (tIndex End = 0; End < AREA_3X4_INDICES; ++End)
        {
            tIndex Start = AREA_3X4_EXIT_INDEX(StartExit);
            tSize Length = 0U;
            uint16_t SecondaryPath = 0U;

            // if primary path length + end path > max path length, keep path
            if (BitTest16(Data, Start)
                AND BitTest16(Data, End)
                AND area_3x4_longest_path(Data, Start, End, &Length, &SecondaryPath)
                AND PrimaryPathLength + Length > MaxLength
                AND area_3x4_exits_open(Original, PrimaryPath) >= MaxPathExitsOpen)
            {
                //printf("DATA\n");
                //area_3x4_print(PrimaryPath | Data);
                //printf("PRIMARY\n");
                //area_3x4_print(PrimaryPath);
                //printf("SECONDARY\n");
                //area_3x4_print(SecondaryPath);
                PathFound = true;
                goto Done;
            }
        }
    }

    // only keep paths with longest through-paths
    // only keep non-adjacent through-paths

    // only need through-paths / end-paths that open more exits??

Done:
    return PathFound;
}

// args need to be signed
static bool area_3x4_exit_adjacent(int Start, int End)
{
    return ((Start <= 2 AND End <= 2) OR (Start >= 3 AND End >= 3)); //OR (Start == 2 AND End == 6) OR (Start == 6 AND End == 2);
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

// calculate longest exitable path?
static tSize area_3x4_longest_exit_path(uint16_t Data)
{
    tSize PathLength, MaxPathLength = 0;

    for (tIndex Exit = 0; Exit < AREA_3X4_EXITS; ++Exit)
    {
        tIndex Index = AREA_3X4_EXIT_INDEX(Exit);

        if (BitTest16(Data, Index)) // adding this if statement adds 1 path hahaha
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

    uint16_t SecArea = Area & ~Path, SecSubArea = Area & ~SubPath;

    for (tIndex StartExit = 0; StartExit < AREA_3X4_EXITS; ++StartExit)
    {
        for (tIndex EndExit = 0; EndExit < AREA_3X4_EXITS; ++EndExit)
        {
            if (StartExit != EndExit)
            {
                tIndex Start = AREA_3X4_EXIT_INDEX(StartExit), End = AREA_3X4_EXIT_INDEX(EndExit);

                uint16_t SecPath = 0U, SubSecPath = 0U;
                tSize SecPathLen = 0U, SubSecPathLen = 0U;
                
                bool SecHas = area_3x4_longest_path(SecArea, Start, End, &SecPathLen, &SecPath);
                bool SubSecHas = area_3x4_longest_path(SecSubArea, Start, End, &SubSecPathLen, &SubSecPath);

                if (SecHas AND NOT SubSecHas)
                {
                    printf("something weird happened... subpath_has_longer_through_path\n");
                    exit(1);
                }

                if (SecHas AND SubSecHas AND SubSecPathLen > SecPathLen)
                {
                    HasLonger = true;
                    break;
                }
            }
        }
    }

    return HasLonger;
}

void area_3x4_area(uint16_t Data, tIndex Index, uint16_t *pArea)
{
    BitReset16(&Data, Index);
    BitSet16(pArea, Index);

    if (LEFT_VALID(Index) AND BitTest16(Data, LEFT(Index))) area_3x4_area(Data, LEFT(Index), pArea);
    if (RIGHT_VALID(Index) AND BitTest16(Data, RIGHT(Index))) area_3x4_area(Data, RIGHT(Index), pArea);
    if (TOP_VALID(Index) AND BitTest16(Data, TOP(Index))) area_3x4_area(Data, TOP(Index), pArea);
    if (BOTTOM_VALID(Index) AND BitTest16(Data, BOTTOM(Index))) area_3x4_area(Data, BOTTOM(Index), pArea);
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

static void area_3x4_print(uint16_t Data)
{
    for (tIndex i = 0; i < AREA_3X4_INDICES; ++i)
    {
        printf("[%c]", BitTest16(Data, i) ? 'X' : ' ');
        if ((i + 1) % 4 == 0) printf("\n");
    }
    printf("\n");
}

static void area_7x7_print(uint64_t Data)
{
    tBoard Board;

    board_init(&Board);

    Board.Data = Data;
    Board.Empty = 0ULL;

    char *Str = board_string(&Board);

    printf("%s\n\n", Str);

    free(Str);
}
