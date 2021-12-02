#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitutil.h"
#include "board.h"
#include "debug.h"
#include "types.h"
#include "util.h"

static const uint64_t BOARD_MASK = 0x0001FFFFFFFFFFFFULL;

static const tIndex BOARD_LAST_MOVE_INDEX   = 56;
static const tSize  BOARD_MIN_ADJ_SQS       = 4;

static const float BOARD_WIN    = 1.0f;
static const float BOARD_DRAW   = 0.5f;
static const float BOARD_LOSS   = 0.0f;

static const float BOARD_WIN_BASE       = 0.90f;
static const float BOARD_WIN_BONUS      = 0.025f;
static const float BOARD_LOSS_BASE      = 0.10f;
static const float BOARD_LOSS_PENALTY   = 0.025f;

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

#define LEFT_VALID(i)   (IndexLookup[i].LeftValid)
#define RIGHT_VALID(i)  (IndexLookup[i].RightValid)
#define TOP_VALID(i)    (IndexLookup[i].TopValid)
#define BOTTOM_VALID(i) (IndexLookup[i].BottomValid)
#define LEFT(i)         (IndexLookup[i].Left)
#define RIGHT(i)        (IndexLookup[i].Right)
#define TOP(i)          (IndexLookup[i].Top)
#define BOTTOM(i)       (IndexLookup[i].Bottom)

static tScore board_optimal_score(tBoard *pBoard);
static tScore board_quick_score(tBoard *pBoard);

static tSize board_index_longest_path(tBoard *pBoard, tIndex Index, uint64_t Checked);
static tSize board_index_checked_path(tBoard *pBoard, tIndex Index, uint64_t Checked, uint64_t *pPath);

static inline bool board_index_has_neighbor(tBoard *pBoard, tIndex Index);
static inline bool board_index_empty(tBoard *pBoard, tIndex Index);
static inline bool board_index_player(tBoard *pBoard, tIndex Index);
static inline tSize board_index_adjacent(uint64_t Checked, tIndex Index);

static inline bool board_traverse_left(tBoard *pBoard, tIndex Index, bool Player, uint64_t Checked);
static inline bool board_traverse_right(tBoard *pBoard, tIndex Index, bool Player, uint64_t Checked);
static inline bool board_traverse_top(tBoard *pBoard, tIndex Index, bool Player, uint64_t Checked);
static inline bool board_traverse_bottom(tBoard *pBoard, tIndex Index, bool Player, uint64_t Checked);

void board_init(tBoard *pBoard)
{
    pBoard->Data = 0ULL;
    pBoard->Valid = 0ULL;
}

void board_copy(tBoard *pDest, tBoard *pSrc)
{
    *pDest = *pSrc;
}

bool board_equals(tBoard *pBoard, tBoard *pB)
{
    return pBoard->Data == pB->Data AND pBoard->Valid == pB->Valid;
}

void board_make_move(tBoard *pBoard, tIndex Index, bool Player)
{
    if (NOT board_index_empty(pBoard, Index))
    {
        dbg_printf(DEBUG_LEVEL_ERROR, "Making invalid move\n");
    }

    if (Player) 
    {
        BitSet64(&pBoard->Data, Index);
    }

    BitSet64(&pBoard->Valid, Index);
    uint64_t IndexShift = (uint64_t) Index << BOARD_LAST_MOVE_INDEX;
    pBoard->Data = IndexShift | (pBoard->Data & BOARD_MASK);
}

bool board_finished(tBoard *pBoard)
{
    return board_move(pBoard) >= ROWS*COLUMNS;
}

tSize board_move(tBoard *pBoard)
{
    return BitPopCount64(pBoard->Valid);
}

tIndex board_last_move_index(tBoard *pBoard)
{
    return pBoard->Data >> BOARD_LAST_MOVE_INDEX;
}

bool board_index_valid(tIndex Index)
{
    return Index >= 0 AND Index < ROWS*COLUMNS;
}

uint64_t board_valid_indices(tBoard *pBoard, uint64_t Policy, bool OnlyAdjacent)
{
    uint64_t Indices = Policy & BOARD_MASK;
    uint64_t ValidIndices = 0ULL, ValidAdjacentIndices = 0ULL;

    while (Indices != 0ULL) 
    {
        uint64_t Tmp = Indices & -Indices;
        uint64_t Index = BitLzCount64(Tmp);

        if (board_index_empty(pBoard, Index)) 
        {
            BitSet64(&ValidIndices, Index);

            if (OnlyAdjacent AND board_index_has_neighbor(pBoard, Index)) 
            {
                BitSet64(&ValidAdjacentIndices, Index);
            }
        }

        Indices ^= Tmp;
    }

    tSize Adjacent = BitPopCount64(ValidAdjacentIndices);

    return IF (OnlyAdjacent AND Adjacent > BOARD_MIN_ADJ_SQS)
        THEN ValidAdjacentIndices
        ELSE ValidIndices;
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
                dbg_printf(DEBUG_LEVEL_WARN, "Cannot quick score if game not finished\n");
            }
            else
            {
                Score = board_quick_score(pBoard);
            }
            break;
        }
        default:
        {
            dbg_printf(DEBUG_LEVEL_WARN, "Invalid scoring algorithm\n");
            break;
        }
    }

    return Score;
}

static tScore board_optimal_score(tBoard *pBoard)
{
    tScore ScoreX = 1, ScoreO = 1;

    for (tIndex i = 0; i < ROWS*COLUMNS; ++i)
    {
        if (NOT board_index_empty(pBoard, i))
        {
            uint64_t Checked = 0ULL;
            tSize ScoreSq = board_index_longest_path(pBoard, i, Checked);

            if (board_index_player(pBoard, i)) 
            {
                SET_IF_GREATER(ScoreSq, ScoreX);
            }
            else 
            {
                SET_IF_GREATER(ScoreSq, ScoreO);
            }
        }
    }

    return ScoreX - ScoreO;
}

static tScore board_quick_score(tBoard *pBoard)
{
    tSize ScoreX = 1, ScoreO = 1;
    uint64_t Checked = 0ULL;

    for (tIndex Index = 0; Index < ROWS*COLUMNS; ++Index)
    {
        bool Player = board_index_player(pBoard, Index);

        if (board_index_adjacent(IF Player THEN pBoard->Data ELSE ~pBoard->Data, Index) <= 1)
        {
            tSize Score = board_index_checked_path(pBoard, Index, 0ULL, &Checked);

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

    for (tIndex Index = 0; Index < ROWS*COLUMNS; ++Index)
    {
        if (NOT BitTest64(Checked, Index))
        {
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
    }

    return ScoreX - ScoreO;
}

float board_fscore(tScore Score)
{
    float Res;
    
    if (Score > 0) 
    {
        float Bonus = Score * BOARD_WIN_BONUS;
        Res = IF (BOARD_WIN_BASE + Bonus > BOARD_WIN)
            THEN BOARD_WIN ELSE (BOARD_WIN_BASE + Bonus);
    }
    else if (Score < 0)
    {
        float Penalty = Score * BOARD_LOSS_PENALTY;
        Res = IF (BOARD_LOSS_BASE + Penalty < BOARD_LOSS)
            THEN BOARD_LOSS ELSE (BOARD_LOSS_BASE + Penalty);
    }
    else
    {
        Res = BOARD_DRAW;
    } 

    return Res;
}

char board_index_char(tBoard *pBoard, tIndex Index)
{
    return IF board_index_empty(pBoard, Index) THEN ' '
        ELSE IF board_index_player(pBoard, Index) THEN 'X' ELSE 'O';
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
    if (NOT board_index_valid(Index))
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Invalid index\n");
    }

    char *Str = malloc(sizeof(char)*BOARD_ID_STR_LEN);
    if (Str IS NULL)
    {
        dbg_printf(DEBUG_LEVEL_ERROR, "No memory available\n");
        goto Error;
    }

    sprintf(Str, "%c%c", (Index%ROWS)+'a', (ROWS-(Index/ROWS))+'0');

Error:
    return Str;
}

char *board_string(tBoard *pBoard)
{
    char *Str = malloc(sizeof(char)*BOARD_STR_LEN);
    if (Str IS NULL)
    {
        dbg_printf(DEBUG_LEVEL_ERROR, "No memory available\n");
        goto Error;
    }

    char *pBegin = Str;

    for (tIndex Index = 0; Index < ROWS*COLUMNS; ++Index)
    {
        if (Index % ROWS == 0)
        {
            Str += sprintf(Str, "%d ", ROWS-Index/ROWS);
        }

        char *SqFmt = IF ((Index+1) % COLUMNS == 0) THEN "[%c]\n" ELSE "[%c]";
        Str += sprintf(Str, SqFmt, board_index_char(pBoard, Index));
    }

    Str += sprintf(Str, "& ");

    for (tIndex Col = 0; Col < COLUMNS; ++Col)
    {
        Str += sprintf(Str, " %c ", 'a'+Col);
    }

    Str = pBegin;

Error:
    return Str;
}

static tSize board_index_longest_path(tBoard *pBoard, tIndex Index, uint64_t Checked)
{
    tSize MaxPathLen = 0;
    bool Player = board_index_player(pBoard, Index);
    BitSet64(&Checked, Index);

    if (board_traverse_left(pBoard, Index, Player, Checked))
    {
        tSize PathLen = board_index_longest_path(pBoard, LEFT(Index), Checked);
        SET_IF_GREATER(PathLen, MaxPathLen);
    }
    if (board_traverse_right(pBoard, Index, Player, Checked))
    {
        tSize PathLen = board_index_longest_path(pBoard, RIGHT(Index), Checked);
        SET_IF_GREATER(PathLen, MaxPathLen);
    }
    if (board_traverse_top(pBoard, Index, Player, Checked))
    {
        tSize PathLen = board_index_longest_path(pBoard, TOP(Index), Checked);
        SET_IF_GREATER(PathLen, MaxPathLen);
    }
    if (board_traverse_bottom(pBoard, Index, Player, Checked))
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

    if (board_traverse_left(pBoard, Index, Player, Checked))
    {
        uint64_t Path = ThisPath;
        tSize PathLen = board_index_checked_path(pBoard, LEFT(Index), Checked, &Path);
        SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Path, MaxPath);
    }
    if (board_traverse_right(pBoard, Index, Player, Checked))
    {
        uint64_t Path = ThisPath;
        tSize PathLen = board_index_checked_path(pBoard, RIGHT(Index), Checked, &Path);
        SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Path, MaxPath);
    }
    if (board_traverse_top(pBoard, Index, Player, Checked))
    {
        uint64_t Path = ThisPath;
        tSize PathLen = board_index_checked_path(pBoard, TOP(Index), Checked, &Path);
        SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Path, MaxPath);
    }
    if (board_traverse_bottom(pBoard, Index, Player, Checked))
    {
        uint64_t Path = ThisPath;
        tSize PathLen = board_index_checked_path(pBoard, BOTTOM(Index), Checked, &Path);
        SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Path, MaxPath);
    }
    
    *pPath |= MaxPath;
    return 1 + MaxPathLen;
}

static inline bool board_index_has_neighbor(tBoard *pBoard, tIndex Index)
{
    if (NOT board_index_valid(Index))
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Invalid index\n");
    }

    return (LEFT_VALID(Index) AND NOT board_index_empty(pBoard, LEFT(Index))) 
        OR (RIGHT_VALID(Index) AND NOT board_index_empty(pBoard, RIGHT(Index))) 
        OR (TOP_VALID(Index) AND NOT board_index_empty(pBoard, TOP(Index))) 
        OR (BOTTOM_VALID(Index) AND NOT board_index_empty(pBoard, BOTTOM(Index))) 
        OR (LEFT_VALID(Index) AND TOP_VALID(Index) AND NOT board_index_empty(pBoard, LEFT(TOP(Index)))) 
        OR (LEFT_VALID(Index) AND BOTTOM_VALID(Index) AND NOT board_index_empty(pBoard, LEFT(BOTTOM(Index)))) 
        OR (RIGHT_VALID(Index) AND TOP_VALID(Index) AND NOT board_index_empty(pBoard, RIGHT(TOP(Index)))) 
        OR (RIGHT_VALID(Index) AND BOTTOM_VALID(Index) AND NOT board_index_empty(pBoard, RIGHT(BOTTOM(Index))));
}

static inline bool board_index_empty(tBoard *pBoard, tIndex Index)
{
    if (NOT board_index_valid(Index))
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Invalid index\n");
    }

    return NOT BitTest64(pBoard->Valid, Index);
}

static inline bool board_index_player(tBoard *pBoard, tIndex Index)
{
    if (board_index_empty(pBoard, Index))
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Index empty\n");
    }

    return BitTest64(pBoard->Data, Index);
}

static inline tSize board_index_adjacent(uint64_t Checked, tIndex Index)
{
    return (LEFT_VALID(Index) && BitTest64(Checked, LEFT(Index)))
        + (RIGHT_VALID(Index) && BitTest64(Checked, RIGHT(Index)))
        + (TOP_VALID(Index) && BitTest64(Checked, TOP(Index)))
        + (BOTTOM_VALID(Index) && BitTest64(Checked, BOTTOM(Index)));
}

static inline bool board_traverse_left(tBoard *pBoard, tIndex Index, bool Player, uint64_t Checked)
{
    return LEFT_VALID(Index) AND NOT board_index_empty(pBoard, LEFT(Index)) 
        AND board_index_player(pBoard, LEFT(Index)) == Player 
        AND NOT BitTest64(Checked, LEFT(Index));
}

static inline bool board_traverse_right(tBoard *pBoard, tIndex Index, bool Player, uint64_t Checked)
{
    return RIGHT_VALID(Index) AND NOT board_index_empty(pBoard, RIGHT(Index)) 
        AND board_index_player(pBoard, RIGHT(Index)) == Player 
        AND NOT BitTest64(Checked, RIGHT(Index));
}

static inline bool board_traverse_top(tBoard *pBoard, tIndex Index, bool Player, uint64_t Checked)
{
    return TOP_VALID(Index) AND NOT board_index_empty(pBoard, TOP(Index)) 
        AND board_index_player(pBoard, TOP(Index)) == Player  
        AND NOT BitTest64(Checked, TOP(Index));
}

static inline bool board_traverse_bottom(tBoard *pBoard, tIndex Index, bool Player, uint64_t Checked)
{
    return BOTTOM_VALID(Index) AND NOT board_index_empty(pBoard, BOTTOM(Index)) 
        AND board_index_player(pBoard, BOTTOM(Index)) == Player 
        AND NOT BitTest64(Checked, BOTTOM(Index));
}
