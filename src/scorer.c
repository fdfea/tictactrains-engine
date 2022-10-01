#include <stdbool.h>
#include <stdint.h>

#include "bitutil.h"
#include "debug.h"
#include "scorer.h"
#include "scorer_lookup_table.h"
#include "types.h"
#include "util.h"

#define AREA_1X1_EXITS      4
#define AREA_1X1_INDICES    1
#define AREA_1X1_MASK       0x0001U

static const tIndex BoardAreaIndicesQ0[AREA_1X1_INDICES] = { 24 };
static const tIndex BoardAreaIndicesQ1[AREA_3X4_INDICES] = {  0,  1,  2,  3,  7,  8,  9, 10, 14, 15, 16, 17 };
static const tIndex BoardAreaIndicesQ2[AREA_3X4_INDICES] = {  6, 13, 20, 27,  5, 12, 19, 26,  4, 11, 18, 25 };
static const tIndex BoardAreaIndicesQ3[AREA_3X4_INDICES] = { 48, 47, 46, 45, 41, 40, 39, 38, 34, 33, 32, 31 };
static const tIndex BoardAreaIndicesQ4[AREA_3X4_INDICES] = { 42, 35, 28, 21, 43, 36, 29, 22, 44, 37, 30, 23 };

static const tIndex BoardAreaExitIndicesQ0[AREA_1X1_EXITS] = { 17, 25, 31, 23 };
static const tIndex BoardAreaExitIndicesQ1[AREA_3X4_EXITS] = {  4, 11, 18, 21, 22, 23, 24 };
static const tIndex BoardAreaExitIndicesQ2[AREA_3X4_EXITS] = { 34, 33, 32,  3, 10, 17, 24 };
static const tIndex BoardAreaExitIndicesQ3[AREA_3X4_EXITS] = { 44, 37, 30, 27, 26, 25, 24 };
static const tIndex BoardAreaExitIndicesQ4[AREA_3X4_EXITS] = { 14, 15, 16, 45, 38, 31, 24 };

static const tIndex board_area_index_c[ROWS*COLUMNS] = {
     0,  1,  2,  3,  8,  4,  0, 
     4,  5,  6,  7,  9,  5,  1,
     8,  9, 10, 11, 10,  6,  2,
     3,  7, 11,  0, 11,  7,  3,
     2,  6, 10, 11, 10,  9,  8,
     1,  5,  9,  7,  6,  5,  4, 
     0,  4,  8,  3,  2,  1,  0,
};

static tSize board_lookup_longest_path(tIndex Index, uint64_t Data);
static tSize area_3x4_lookup_longest_path(tIndex Index, uint64_t Data);
static tSize area_1x1_lookup_longest_path(tIndex Index, uint64_t Data);

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

static tIndex board_area_index_q0(tIndex Index);
static tIndex board_area_index_q1(tIndex Index);
static tIndex board_area_index_q2(tIndex Index);
static tIndex board_area_index_q3(tIndex Index);
static tIndex board_area_index_q4(tIndex Index);

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

static const tIndex (*board_area_index_q[ROWS*COLUMNS])(tIndex) = {
    board_area_index_q1, board_area_index_q1, board_area_index_q1, board_area_index_q1, board_area_index_q2, board_area_index_q2, board_area_index_q2,
    board_area_index_q1, board_area_index_q1, board_area_index_q1, board_area_index_q1, board_area_index_q2, board_area_index_q2, board_area_index_q2,
    board_area_index_q1, board_area_index_q1, board_area_index_q1, board_area_index_q1, board_area_index_q2, board_area_index_q2, board_area_index_q2,
    board_area_index_q4, board_area_index_q4, board_area_index_q4, board_area_index_q0, board_area_index_q2, board_area_index_q2, board_area_index_q2,
    board_area_index_q4, board_area_index_q4, board_area_index_q4, board_area_index_q3, board_area_index_q3, board_area_index_q3, board_area_index_q3,
    board_area_index_q4, board_area_index_q4, board_area_index_q4, board_area_index_q3, board_area_index_q3, board_area_index_q3, board_area_index_q3,
    board_area_index_q4, board_area_index_q4, board_area_index_q4, board_area_index_q3, board_area_index_q3, board_area_index_q3, board_area_index_q3,
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

static const tSize (*area_lookup_longest_path[ROWS*COLUMNS])(tIndex, uint64_t) = {
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_1x1_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
    area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path, area_3x4_lookup_longest_path,
};

tScore score(tBoard *pBoard)
{
    tScore ScoreX = 0, ScoreO = 0;

    for (tIndex Index = 0; Index < ROWS*COLUMNS; ++Index)
    {
        bool Player = board_index_player(pBoard, Index);
        uint64_t Data = IF Player THEN pBoard->Data ELSE ~pBoard->Data & BOARD_MASK;
        tScore Score = board_lookup_longest_path(Index, Data);

        if (Player)
        {
            //printf("board_lookup_score -- index: %d, player: X, score: %d\n", Index, Score);
            SET_IF_GREATER(Score, ScoreX);
        }
        else
        {
            //printf("board_lookup_score -- index: %d, player: O, score: %d\n", Index, Score);
            SET_IF_GREATER(Score, ScoreO);
        }
    }

    //printf("board_lookup_score -- max score x: %d, max score o: %d\n", ScoreX, ScoreO);

    return ScoreX - ScoreO;
}

static tSize board_lookup_longest_path(tIndex Index, uint64_t Data)
{
    return (*area_lookup_longest_path[Index])(Index, Data);
}

static tSize area_3x4_lookup_longest_path(tIndex Index, uint64_t Data)
{
    const tArea3x4Lookup *pAreaLookup = scorer_area_3x4_lookup((*extract_q[Index])(Data));
    const tArea3x4IndexLookup *pAreaIndexLookup = &pAreaLookup->IndexPaths[board_area_index_c[Index]];
    tSize MaxPathLength = 0;

    for (tIndex i = 0; i < AREA_3X4_EXITS; ++i)
    {
        const tArea3x4PathLookup* pAreaPathLookup = &pAreaIndexLookup->ExitPaths[i];
        tIndex NextIndex = (*board_area_exit_index_q[Index])(i);

        SET_IF_GREATER(pAreaIndexLookup->LongestPath, MaxPathLength);

        if (BitTest64(Data, NextIndex))
        {
            tSize LongestPathLength = BitPopCount64(pAreaPathLookup->LongestPath & AREA_3X4_MASK);
            //tSize LeastExitPathLength = BitPopCount64(pAreaPathLookup->LeastExitPath & AREA_3X4_MASK);
            //tSize ShortestPathLength = BitPopCount64(pAreaPathLookup->ShortestPath & AREA_3X4_MASK);

            if (LongestPathLength > 0)
            {
                uint64_t LongestPathConverted = (*convert_q[Index])(pAreaPathLookup->LongestPath);
                uint64_t NextData = Data & ~LongestPathConverted;
                //printf("area_3x4_lookup_longest_path -- index: %d, next index longest: %d\n", Index, NextIndex);
                tSize PathLength = LongestPathLength + board_lookup_longest_path(NextIndex, NextData);
                SET_IF_GREATER(PathLength, MaxPathLength);
            }

            /*
            if (LeastExitPathLength > 0)
            {
                uint64_t LeastExitPathConverted = (*convert_q[Index])(pAreaPathLookup->LeastExitPath);
                uint64_t NextData = Data & ~LeastExitPathConverted;
                //printf("area_3x4_lookup_longest_path -- index: %d, next index least exit: %d\n", Index, NextIndex);
                tSize PathLength = LeastExitPathLength + board_lookup_longest_path(NextIndex, NextData);
                SET_IF_GREATER(PathLength, MaxPathLength);
            }
            */

            /*
            if (ShortestPathLength > 0)
            {
                uint64_t ShortestPathConverted = (*convert_q[Index])(pAreaPathLookup->ShortestPath);
                uint64_t NextData = Data & ~ShortestPathConverted;
                //printf("area_3x4_lookup_longest_path -- index: %d, next index shortest: %d\n", Index, NextIndex);
                tSize PathLength = ShortestPathLength + board_lookup_longest_path(NextIndex, NextData);
                SET_IF_GREATER(PathLength, MaxPathLength);
            }
            */
        }
    }

    //printf("area_3x4_lookup_longest_path -- index: %d, max length: %d\n", Index, MaxPathLength);

    return MaxPathLength;
}

static tSize area_1x1_lookup_longest_path(tIndex Index, uint64_t Data)
{
    tSize MaxPathLength = 0;

    BitReset64(&Data, Index);

    for (tIndex i = 0; i < AREA_1X1_EXITS; ++i)
    {
        tIndex ExitIndex = (*board_area_exit_index_q[Index])(i);

        if(BitTest64(Data, ExitIndex))
        {
            //printf("area_1x1_lookup_longest_path -- index: %d, next index: %d\n", Index, ExitIndex);
            tSize PathLength = board_lookup_longest_path(ExitIndex, Data);
            SET_IF_GREATER(PathLength, MaxPathLength);
        }
    }

    //printf("area_1x1_lookup_longest_path -- index: %d, max length: %d\n", Index, 1 + MaxPathLength);

    return 1 + MaxPathLength;
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

static tIndex board_area_index_q0(tIndex Index)
{
    return BoardAreaIndicesQ0[Index];
}

static tIndex board_area_index_q1(tIndex Index)
{
    return BoardAreaIndicesQ1[Index];
}

static tIndex board_area_index_q2(tIndex Index)
{
    return BoardAreaIndicesQ2[Index];
}

static tIndex board_area_index_q3(tIndex Index)
{
    return BoardAreaIndicesQ3[Index];
}

static tIndex board_area_index_q4(tIndex Index)
{
    return BoardAreaIndicesQ4[Index];
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
