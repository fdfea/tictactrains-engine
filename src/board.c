#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bitutil.h"
#include "board.h"
#include "debug.h"
#include "types.h"
#include "util.h"

#define BOARD_MASK  0x0001FFFFFFFFFFFFULL

#define BOARD_LAST_MOVE_INDEX       56
#define BOARD_MIN_NEIGHBOR_INDICES  6

static const uint64_t IndexLookup[ROWS*COLUMNS][2] = {
    { 0x0000000000000082ULL, 0x0000000000000182ULL },
    { 0x0000000000000105ULL, 0x0000000000000385ULL },
    { 0x000000000000020AULL, 0x000000000000070AULL },
    { 0x0000000000000414ULL, 0x0000000000000E14ULL },
    { 0x0000000000000828ULL, 0x0000000000001C28ULL },
    { 0x0000000000001050ULL, 0x0000000000003850ULL },
    { 0x0000000000002020ULL, 0x0000000000003020ULL },
    { 0x0000000000004101ULL, 0x000000000000C103ULL },
    { 0x0000000000008282ULL, 0x000000000001C287ULL },
    { 0x0000000000010504ULL, 0x000000000003850EULL },
    { 0x0000000000020A08ULL, 0x0000000000070A1CULL },
    { 0x0000000000041410ULL, 0x00000000000E1438ULL },
    { 0x0000000000082820ULL, 0x00000000001C2870ULL },
    { 0x0000000000101040ULL, 0x0000000000181060ULL },
    { 0x0000000000208080ULL, 0x0000000000608180ULL },
    { 0x0000000000414100ULL, 0x0000000000E14380ULL },
    { 0x0000000000828200ULL, 0x0000000001C28700ULL },
    { 0x0000000001050400ULL, 0x0000000003850E00ULL },
    { 0x00000000020A0800ULL, 0x00000000070A1C00ULL },
    { 0x0000000004141000ULL, 0x000000000E143800ULL },
    { 0x0000000008082000ULL, 0x000000000C083000ULL },
    { 0x0000000010404000ULL, 0x000000003040C000ULL },
    { 0x0000000020A08000ULL, 0x0000000070A1C000ULL },
    { 0x0000000041410000ULL, 0x00000000E1438000ULL },
    { 0x0000000082820000ULL, 0x00000001C2870000ULL },
    { 0x0000000105040000ULL, 0x00000003850E0000ULL },
    { 0x000000020A080000ULL, 0x000000070A1C0000ULL },
    { 0x0000000404100000ULL, 0x0000000604180000ULL },
    { 0x0000000820200000ULL, 0x0000001820600000ULL },
    { 0x0000001050400000ULL, 0x0000003850E00000ULL },
    { 0x00000020A0800000ULL, 0x00000070A1C00000ULL },
    { 0x0000004141000000ULL, 0x000000E143800000ULL },
    { 0x0000008282000000ULL, 0x000001C287000000ULL },
    { 0x0000010504000000ULL, 0x000003850E000000ULL },
    { 0x0000020208000000ULL, 0x000003020C000000ULL },
    { 0x0000041010000000ULL, 0x00000C1030000000ULL },
    { 0x0000082820000000ULL, 0x00001C2870000000ULL },
    { 0x0000105040000000ULL, 0x00003850E0000000ULL },
    { 0x000020A080000000ULL, 0x000070A1C0000000ULL },
    { 0x0000414100000000ULL, 0x0000E14380000000ULL },
    { 0x0000828200000000ULL, 0x0001C28700000000ULL },
    { 0x0001010400000000ULL, 0x0001810600000000ULL },
    { 0x0000080800000000ULL, 0x0000081800000000ULL },
    { 0x0000141000000000ULL, 0x0000143800000000ULL },
    { 0x0000282000000000ULL, 0x0000287000000000ULL },
    { 0x0000504000000000ULL, 0x000050E000000000ULL },
    { 0x0000A08000000000ULL, 0x0000A1C000000000ULL },
    { 0x0001410000000000ULL, 0x0001438000000000ULL },
    { 0x0000820000000000ULL, 0x0000830000000000ULL },
};

#define ADJACENT_INDICES(i)     (IndexLookup[i][0])
#define NEIGHBOR_INDICES(i)     (IndexLookup[i][1])

static tScore board_optimal_score(tBoard *pBoard);
static tScore board_quick_score(tBoard *pBoard);

static bool board_index_empty(tBoard *pBoard, tIndex Index);
static bool board_index_player(tBoard *pBoard, tIndex Index);
static bool board_index_traversable(tBoard *pBoard, tIndex Index, bool Player, uint64_t Checked);

static tSize board_index_longest_path(tBoard *pBoard, tIndex Index, uint64_t Checked);
static tSize board_index_checked_path(tBoard *pBoard, tIndex Index, uint64_t Checked, uint64_t *pPath);
static tSize board_index_adjacent_count(uint64_t Checked, tIndex Index);

void board_init(tBoard *pBoard)
{
    pBoard->Data = 0ULL;
    pBoard->Empty = UINT64_MAX & BOARD_MASK;
    pBoard->Neighbors = 0ULL;
}

void board_copy(tBoard *pBoard, tBoard *pB)
{
    *pBoard = *pB;
}

bool board_equals(tBoard *pBoard, tBoard *pB)
{
    return pBoard->Data == pB->Data AND pBoard->Empty == pB->Empty;
}

int board_advance(tBoard *pBoard, tIndex Index, bool Player)
{
    int Res = 0;

    if (NOT board_index_empty(pBoard, Index))
    {
        Res = -EINVAL;
        dbg_printf(DEBUG_LEVEL_ERROR, "Cannot make move at invalid index");
        goto Error;
    }

    if (Player)
    {
        BitSet64(&pBoard->Data, Index);
    }

    BitReset64(&pBoard->Empty, Index);

    pBoard->Neighbors = (pBoard->Neighbors | NEIGHBOR_INDICES(Index)) & pBoard->Empty;
    pBoard->Data = (pBoard->Data & BOARD_MASK) | (uint64_t) Index << BOARD_LAST_MOVE_INDEX;

Error:
    return Res;
}

bool board_finished(tBoard *pBoard)
{
    return board_move(pBoard) >= ROWS*COLUMNS;
}

tSize board_move(tBoard *pBoard)
{
    return BitPopCount64(~pBoard->Empty & BOARD_MASK);
}

bool board_index_valid(tIndex Index)
{
    return Index >= 0 AND Index < ROWS*COLUMNS;
}

uint64_t board_empty_indices(tBoard *pBoard, uint64_t Constraint, bool OnlyNeighbors)
{
    uint64_t Empty = pBoard->Empty & Constraint;

    if (OnlyNeighbors)
    {
        uint64_t Neighbors = pBoard->Neighbors & Constraint;

        if (BitPopCount64(Neighbors) >= BOARD_MIN_NEIGHBOR_INDICES)
        {
            Empty = Neighbors;
        }
    }

    return Empty;
}

tIndex board_last_move_index(tBoard *pBoard)
{
    return pBoard->Data >> BOARD_LAST_MOVE_INDEX;
}

tScore board_score(tBoard *pBoard, eScoringAlgorithm Algorithm)
{
    tScore Score = 0;

    switch (Algorithm)
    {
        case SCORING_ALGORITHM_OPTIMAL:
        {
            Score = board_optimal_score(pBoard);
            break;
        }
        case SCORING_ALGORITHM_QUICK:
        {
            if (NOT board_finished(pBoard))
            {
                dbg_printf(DEBUG_LEVEL_WARN, "Cannot quick score board if game not finished");
            }
            else
            {
                Score = board_quick_score(pBoard);
            }
            break;
        }
        default:
        {
            dbg_printf(DEBUG_LEVEL_WARN, "Cannot score board with invalid scoring algorithm");
            break;
        }
    }

    return Score;
}

static tScore board_optimal_score(tBoard *pBoard)
{
    tScore ScoreX = 0, ScoreO = 0;
    uint64_t NotEmpty = ~pBoard->Empty & BOARD_MASK;

    while (NOT BitEmpty64(NotEmpty))
    {
        tIndex Index = BitTzCount64(NotEmpty);
        tSize Score = board_index_longest_path(pBoard, Index, 0ULL);

        if (board_index_player(pBoard, Index)) 
        {
            SET_IF_GREATER(Score, ScoreX);
        }
        else
        {
            SET_IF_GREATER(Score, ScoreO);
        }

        BitReset64(&NotEmpty, Index);
    }

    return ScoreX - ScoreO;
}

static tScore board_quick_score(tBoard *pBoard)
{
    tSize ScoreX = 0, ScoreO = 0;
    uint64_t NotChecked, Checked = 0ULL;

    for (tIndex i = 0; i < ROWS*COLUMNS; ++i)
    {
        bool Player = board_index_player(pBoard, i);

        if (board_index_adjacent_count(IF Player THEN pBoard->Data ELSE ~pBoard->Data, i) <= 1)
        {
            tSize Score = board_index_checked_path(pBoard, i, 0ULL, &Checked);

            if (Player)
            {
                SET_IF_GREATER(Score, ScoreX);
            }
            else
            {
                SET_IF_GREATER(Score, ScoreO);
            }
        }
    }

    while (NOT BitEmpty64(NotChecked = ~Checked & BOARD_MASK))
    {
        tIndex Index = BitTzCount64(NotChecked);
        tSize Score = board_index_checked_path(pBoard, Index, 0ULL, &Checked);

        if (board_index_player(pBoard, Index))
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

char board_index_char(tBoard *pBoard, tIndex Index)
{
    return IF board_index_empty(pBoard, Index) THEN ' '
        ELSE IF board_index_player(pBoard, Index) THEN 'X'
        ELSE 'O';
}

tIndex board_id_index(char (*pId)[BOARD_ID_STR_LEN])
{
    return ROWS*(ROWS-((*pId)[1]-'0'))+((*pId)[0]-'a');
}

bool board_id_valid(char (*pId)[BOARD_ID_STR_LEN])
{
    return (*pId)[0] >= 'a'
        AND (*pId)[0] < 'a' + ROWS
        AND (*pId)[1]-'0' > 0
        AND (*pId)[1]-'0' <= ROWS;
}

char *board_index_id(tIndex Index)
{
    char *Str = NULL;

    if (NOT board_index_valid(Index))
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Cannot convert index to id with invalid index");
        goto Error;
    }

    Str = emalloc(BOARD_ID_STR_LEN * sizeof(char));

    sprintf(Str, "%c%c", (Index%ROWS)+'a', (ROWS-(Index/ROWS))+'0');

Error:
    return Str;
}

char *board_string(tBoard *pBoard)
{
    char *Str = emalloc(BOARD_STR_LEN * sizeof(char)), *pBegin = Str;

    for (tIndex i = 0; i < ROWS*COLUMNS; ++i)
    {
        if (i % ROWS == 0)
        {
            Str += sprintf(Str, "%d ", ROWS-i/ROWS);
        }

        char *SqFmt = IF ((i+1) % COLUMNS == 0) THEN "[%c]\n" ELSE "[%c]";
        
        Str += sprintf(Str, SqFmt, board_index_char(pBoard, i));
    }

    Str += sprintf(Str, "& ");

    for (tIndex Col = 0; Col < COLUMNS; ++Col)
    {
        Str += sprintf(Str, " %c ", 'a'+Col);
    }

    return pBegin;
}

static bool board_index_empty(tBoard *pBoard, tIndex Index)
{
    if (NOT board_index_valid(Index))
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Cannot check if board index out of bounds is empty");
    }

    return BitTest64(pBoard->Empty, Index);
}

static bool board_index_player(tBoard *pBoard, tIndex Index)
{
    if (board_index_empty(pBoard, Index))
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Cannot get player from empty board index");
    }

    return BitTest64(pBoard->Data, Index);
}

static bool board_index_traversable(tBoard *pBoard, tIndex Index, bool Player, uint64_t Checked)
{
    return NOT board_index_empty(pBoard, Index) 
        AND board_index_player(pBoard, Index) == Player 
        AND NOT BitTest64(Checked, Index);
}

static tSize board_index_longest_path(tBoard *pBoard, tIndex Index, uint64_t Checked)
{
    tSize MaxPathLen = 0;
    uint64_t AdjacentIndices = ADJACENT_INDICES(Index);

    BitSet64(&Checked, Index);

    while (NOT BitEmpty64(AdjacentIndices))
    {
        tIndex AdjIndex = BitTzCount64(AdjacentIndices);
    
        if (board_index_traversable(pBoard, AdjIndex, board_index_player(pBoard, Index), Checked))
        {
            tSize PathLen = board_index_longest_path(pBoard, AdjIndex, Checked);
            SET_IF_GREATER(PathLen, MaxPathLen);        
        }

        BitReset64(&AdjacentIndices, AdjIndex);
    }

    return MaxPathLen + 1;
}

static tSize board_index_checked_path(tBoard *pBoard, tIndex Index, uint64_t Checked, uint64_t *pPath)
{
    tSize MaxPathLen = 0;
    uint64_t MaxPath = 0ULL, ThisPath = 0ULL, AdjacentIndices = ADJACENT_INDICES(Index);

    BitSet64(&ThisPath, Index);

    Checked |= MaxPath = ThisPath;

    while (NOT BitEmpty64(AdjacentIndices))
    {
        tIndex AdjIndex = BitTzCount64(AdjacentIndices);
    
        if (board_index_traversable(pBoard, AdjIndex, board_index_player(pBoard, Index), Checked))
        {
            uint64_t Path = ThisPath;
            tSize PathLen = board_index_checked_path(pBoard, AdjIndex, Checked, &Path);
            SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Path, MaxPath);         
        }

        BitReset64(&AdjacentIndices, AdjIndex);
    }
    
    *pPath |= MaxPath;

    return MaxPathLen + 1;
}

static tSize board_index_adjacent_count(uint64_t Data, tIndex Index)
{
    uint64_t AdjacentIndices = ADJACENT_INDICES(Index);
    tSize AdjacentCount = 0;

    while (NOT BitEmpty64(AdjacentIndices))
    {
        tIndex Index = BitTzCount64(AdjacentIndices);
    
        if (BitTest64(Data, Index))
        {
            AdjacentCount++;
        }

        BitReset64(&AdjacentIndices, Index);
    }

    return AdjacentCount;
}
