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

typedef struct IndexLookup
{
    bool LeftValid, RightValid, TopValid, BottomValid;
    tIndex Left, Right, Top, Bottom;
}
tIndexLookup;

static const tIndexLookup IndexLookup[ROWS*COLUMNS] = {
    { false, true , false, true , 0x00, 0x01, 0x00, 0x07 },
    { true , true , false, true , 0x00, 0x02, 0x00, 0x08 },
    { true , true , false, true , 0x01, 0x03, 0x00, 0x09 },
    { true , true , false, true , 0x02, 0x04, 0x00, 0x0a },
    { true , true , false, true , 0x03, 0x05, 0x00, 0x0b },
    { true , true , false, true , 0x04, 0x06, 0x00, 0x0c },
    { true , false, false, true , 0x05, 0x00, 0x00, 0x0d },
    { false, true , true , true , 0x00, 0x08, 0x00, 0x0e },
    { true , true , true , true , 0x07, 0x09, 0x01, 0x0f },
    { true , true , true , true , 0x08, 0x0a, 0x02, 0x10 },
    { true , true , true , true , 0x09, 0x0b, 0x03, 0x11 },
    { true , true , true , true , 0x0a, 0x0c, 0x04, 0x12 },
    { true , true , true , true , 0x0b, 0x0d, 0x05, 0x13 },
    { true , false, true , true , 0x0c, 0x00, 0x06, 0x14 },
    { false, true , true , true , 0x00, 0x0f, 0x07, 0x15 },
    { true , true , true , true , 0x0e, 0x10, 0x08, 0x16 },
    { true , true , true , true , 0x0f, 0x11, 0x09, 0x17 },
    { true , true , true , true , 0x10, 0x12, 0x0a, 0x18 },
    { true , true , true , true , 0x11, 0x13, 0x0b, 0x19 },
    { true , true , true , true , 0x12, 0x14, 0x0c, 0x1a },
    { true , false, true , true , 0x13, 0x00, 0x0d, 0x1b },
    { false, true , true , true , 0x00, 0x16, 0x0e, 0x1c },
    { true , true , true , true , 0x15, 0x17, 0x0f, 0x1d },
    { true , true , true , true , 0x16, 0x18, 0x10, 0x1e },
    { true , true , true , true , 0x17, 0x19, 0x11, 0x1f },
    { true , true , true , true , 0x18, 0x1a, 0x12, 0x20 },
    { true , true , true , true , 0x19, 0x1b, 0x13, 0x21 },
    { true , false, true , true , 0x1a, 0x00, 0x14, 0x22 },
    { false, true , true , true , 0x00, 0x1d, 0x15, 0x23 },
    { true , true , true , true , 0x1c, 0x1e, 0x16, 0x24 },
    { true , true , true , true , 0x1d, 0x1f, 0x17, 0x25 },
    { true , true , true , true , 0x1e, 0x20, 0x18, 0x26 },
    { true , true , true , true , 0x1f, 0x21, 0x19, 0x27 },
    { true , true , true , true , 0x20, 0x22, 0x1a, 0x28 },
    { true , false, true , true , 0x21, 0x00, 0x1b, 0x29 },
    { false, true , true , true , 0x00, 0x24, 0x1c, 0x2a },
    { true , true , true , true , 0x23, 0x25, 0x1d, 0x2b },
    { true , true , true , true , 0x24, 0x26, 0x1e, 0x2c },
    { true , true , true , true , 0x25, 0x27, 0x1f, 0x2d },
    { true , true , true , true , 0x26, 0x28, 0x20, 0x2e },
    { true , true , true , true , 0x27, 0x29, 0x21, 0x2f },
    { true , false, true , true , 0x28, 0x00, 0x22, 0x30 },
    { false, true , true , false, 0x00, 0x2b, 0x23, 0x00 },
    { true , true , true , false, 0x2a, 0x2c, 0x24, 0x00 },
    { true , true , true , false, 0x2b, 0x2d, 0x25, 0x00 },
    { true , true , true , false, 0x2c, 0x2e, 0x26, 0x00 },
    { true , true , true , false, 0x2d, 0x2f, 0x27, 0x00 },
    { true , true , true , false, 0x2e, 0x30, 0x28, 0x00 },
    { true , false, true , false, 0x2f, 0x00, 0x29, 0x00 },
};

static const uint64_t IndicesLookup[ROWS*COLUMNS][2] = {
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

#define LEFT_VALID(i)   (IndexLookup[i].LeftValid)
#define RIGHT_VALID(i)  (IndexLookup[i].RightValid)
#define TOP_VALID(i)    (IndexLookup[i].TopValid)
#define BOTTOM_VALID(i) (IndexLookup[i].BottomValid)
#define LEFT(i)         (IndexLookup[i].Left)
#define RIGHT(i)        (IndexLookup[i].Right)
#define TOP(i)          (IndexLookup[i].Top)
#define BOTTOM(i)       (IndexLookup[i].Bottom)

#define ADJACENT_INDICES(i)     (IndicesLookup[i][0])
#define NEIGHBOR_INDICES(i)     (IndicesLookup[i][1])

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
    bool Player = board_index_player(pBoard, Index);
    BitSet64(&Checked, Index);

    if (LEFT_VALID(Index) AND board_index_traversable(pBoard, LEFT(Index), Player, Checked))
    {
        tSize PathLen = board_index_longest_path(pBoard, LEFT(Index), Checked);
        SET_IF_GREATER(PathLen, MaxPathLen);
    }
    if (RIGHT_VALID(Index) AND board_index_traversable(pBoard, RIGHT(Index), Player, Checked))
    {
        tSize PathLen = board_index_longest_path(pBoard, RIGHT(Index), Checked);
        SET_IF_GREATER(PathLen, MaxPathLen);
    }
    if (TOP_VALID(Index) AND board_index_traversable(pBoard, TOP(Index), Player, Checked))
    {
        tSize PathLen = board_index_longest_path(pBoard, TOP(Index), Checked);
        SET_IF_GREATER(PathLen, MaxPathLen);
    }
    if (BOTTOM_VALID(Index) AND board_index_traversable(pBoard, BOTTOM(Index), Player, Checked))
    {
        tSize PathLen = board_index_longest_path(pBoard, BOTTOM(Index), Checked);
        SET_IF_GREATER(PathLen, MaxPathLen);
    }

    return 1 + MaxPathLen;
}

static tSize board_index_checked_path(tBoard *pBoard, tIndex Index, uint64_t Checked, uint64_t *pPath)
{
    tSize MaxPathLen = 0;
    uint64_t MaxPath, ThisPath = 0ULL;
    bool Player = board_index_player(pBoard, Index);

    BitSet64(&ThisPath, Index);

    Checked |= MaxPath = ThisPath;

    if (LEFT_VALID(Index) AND board_index_traversable(pBoard, LEFT(Index), Player, Checked))
    {
        uint64_t Path = ThisPath;
        tSize PathLen = board_index_checked_path(pBoard, LEFT(Index), Checked, &Path);
        SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Path, MaxPath);
    }
    if (RIGHT_VALID(Index) AND board_index_traversable(pBoard, RIGHT(Index), Player, Checked))
    {
        uint64_t Path = ThisPath;
        tSize PathLen = board_index_checked_path(pBoard, RIGHT(Index), Checked, &Path);
        SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Path, MaxPath);
    }
    if (TOP_VALID(Index) AND board_index_traversable(pBoard, TOP(Index), Player, Checked))
    {
        uint64_t Path = ThisPath;
        tSize PathLen = board_index_checked_path(pBoard, TOP(Index), Checked, &Path);
        SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Path, MaxPath);
    }
    if (BOTTOM_VALID(Index) AND board_index_traversable(pBoard, BOTTOM(Index), Player, Checked))
    {
        uint64_t Path = ThisPath;
        tSize PathLen = board_index_checked_path(pBoard, BOTTOM(Index), Checked, &Path);
        SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Path, MaxPath);
    }
    
    *pPath |= MaxPath;
    return 1 + MaxPathLen;
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
