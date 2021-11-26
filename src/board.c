#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitutil.h"
#include "board.h"
#include "debug.h"
#include "defs.h"
#include "types.h"
#include "util.h"

#define BOARD_MASK              0x0001FFFFFFFFFFFFULL
#define BOARD_LAST_MOVE_INDEX   58
#define BOARD_MIN_ADJ_SQS       5

#define BOARD_WIN   1.0f
#define BOARD_DRAW  0.5f
#define BOARD_LOSS  0.0f

#define BOARD_WIN_BASE      0.90f
#define BOARD_WIN_BONUS     0.025f
#define BOARD_LOSS_BASE     0.10f
#define BOARD_LOSS_PENALTY  0.025f

static tScore board_optimal_score(tBoard *pBoard);
static tScore board_quick_score(tBoard *pBoard);
static tSize board_index_longest_path(tBoard *pBoard, tIndex Index, uint64_t Checked);
static tSize board_index_checked_path(tBoard *pBoard, tIndex Index, uint64_t ChkIn, uint64_t *pChkOut);
static bool board_adj_index_not_empty(tBoard *pBoard, tIndex Index);

static tSize max_size(tSize A, tSize B);

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
        dbg_printf(DEBUG_ERROR, "Making invalid move\n");
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

uint64_t board_valid_indices(tBoard *pBoard, uint64_t Policy, bool Adj)
{
    uint64_t Indices = Policy & BOARD_MASK;
    uint64_t Tmp, ValidIndices = 0ULL, ValidAdjIndices = 0ULL;
    tIndex Index;

    while (Indices != 0ULL) 
    {
        Tmp = Indices & -Indices;
        Index = BitLzCount64(Tmp);

        if (board_index_empty(pBoard, Index)) 
        {
            BitSet64(&ValidIndices, Index);

            if (Adj AND board_adj_index_not_empty(pBoard, Index)) 
            {
                BitSet64(&ValidAdjIndices, Index);
            }
        }

        Indices ^= Tmp;
    }

    tSize AdjLen = BitPopCount64(ValidAdjIndices);

    return (Adj AND AdjLen > BOARD_MIN_ADJ_SQS) 
        ? ValidAdjIndices 
        : ValidIndices;
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
            Score = board_quick_score(pBoard);
            break;
        }
        default:
        {
            dbg_printf(DEBUG_WARN, "Invalid scoring algorithm\n");
            break;
        }
    }

    return Score;
}

static tScore board_optimal_score(tBoard *pBoard)
{
    tScore ScoreSq, ScoreX = 0, ScoreO = 0;

    for (tIndex i = 0; i < ROWS*COLUMNS; ++i) 
    {
        if (NOT board_index_empty(pBoard, i)) 
        {
            uint64_t Checked = 0ULL;
            ScoreSq = board_index_longest_path(pBoard, i, Checked);

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
    tScore ScoreSq, ScoreX = 0, ScoreO = 0;
    uint64_t Checked = 0ULL;

    for (tIndex i = 0; i < ROWS*COLUMNS; ++i) 
    {
        if (NOT board_index_empty(pBoard, i) AND NOT BitTest64(Checked, i)) 
        {
            uint64_t Chkd = 0ULL;
            ScoreSq = board_index_checked_path(pBoard, i, Chkd, &Checked);

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

float board_fscore(tScore Score)
{
    float Res;
    
    if (Score > 0) 
    {
        float Bonus = Score * BOARD_WIN_BONUS;
        Res = BOARD_WIN_BASE + Bonus > BOARD_WIN
            ? BOARD_WIN : BOARD_WIN_BASE + Bonus;
    }
    else if (Score < 0)
    {
        float Penalty = Score * BOARD_LOSS_PENALTY;
        Res = BOARD_LOSS_BASE + Penalty < BOARD_LOSS
            ? BOARD_LOSS : BOARD_LOSS_BASE + Penalty;
    }
    else
    {
        Res = BOARD_DRAW;
    } 

    return Res;
}

tSize board_longest_path(tBoard *pBoard)
{
    tSize LenSq, LongestPath = 0;

    for (tIndex i = 0; i < ROWS*COLUMNS; ++i) 
    {
        if (NOT board_index_empty(pBoard, i)) 
        {
            uint64_t Checked = 0ULL;
            LenSq = board_index_longest_path(pBoard, i, Checked);

            SET_IF_GREATER(LenSq, LongestPath);
        }
    }

    return LongestPath;
}

char board_index_char(tBoard *pBoard, tIndex Index)
{
    return NOT board_index_empty(pBoard, Index) ? board_index_player(pBoard, Index) 
        ? 'X' : 'O' : ' ';
}

tIndex board_id_index(char (*pId)[BOARD_ID_STR_LEN])
{
    return ROWS*(ROWS-((*pId)[1]-'0'))+((*pId)[0]-'a');
}

bool board_id_valid(char (*pId)[BOARD_ID_STR_LEN])
{
    return (
        (*pId)[0] >= 'a'
        AND (*pId)[0] < 'a' + ROWS
        AND (*pId)[1]-'0' > 0
        AND (*pId)[1]-'0' <= ROWS
    );
}

char *board_index_id(tIndex Index)
{
    char *pId = NULL;

    if (NOT board_index_valid(Index))
    {
        dbg_printf(DEBUG_WARN, "Invalid index\n");
    }

    pId = malloc(sizeof(char)*BOARD_ID_STR_LEN);
    if (pId IS NULL)
    {
        dbg_printf(DEBUG_ERROR, "No memory available\n");
        goto Error;
    }

    sprintf(pId, "%c%c", (Index%ROWS)+'a', (ROWS-(Index/ROWS))+'0');

Error:
    return pId;
}

char *board_string(tBoard *pBoard)
{
    char *pStr = NULL;

    pStr = malloc(sizeof(char)*BOARD_STR_LEN);
    if (pStr IS NULL)
    {
        dbg_printf(DEBUG_ERROR, "No memory available\n");
        goto Error;
    }

    char *pBegin = pStr;

    for (tIndex i = 0; i < ROWS*COLUMNS; ++i)
    {
        if (i % ROWS == 0)
        {
            pStr += sprintf(pStr, "%d ", ROWS-i/ROWS);
        }

        pStr += sprintf(pStr, ((i+1) % COLUMNS == 0) 
            ? "[%c]\n" : "[%c]", board_index_char(pBoard, i));
    }

    pStr += sprintf(pStr, "& ");

    for (tIndex col = 0; col < COLUMNS; ++col)
    {
        pStr += sprintf(pStr, " %c ", 'a'+col);
    }

    pStr = pBegin;

Error:
    return pStr;
}

static bool board_adj_index_not_empty(tBoard *pBoard, tIndex Index)
{
    if (NOT board_index_valid(Index))
    {
        dbg_printf(DEBUG_WARN, "Invalid index\n");
    }

    return (
        (board_index_left_valid(Index) 
            AND NOT board_index_empty(pBoard, board_index_left(Index))) 
        OR (board_index_right_valid(Index) 
            AND NOT board_index_empty(pBoard, board_index_right(Index))) 
        OR (board_index_top_valid(Index) 
            AND NOT board_index_empty(pBoard, board_index_top(Index))) 
        OR (board_index_bottom_valid(Index) 
            AND NOT board_index_empty(pBoard, board_index_bottom(Index))) 
        OR (board_index_left_valid(Index) 
            AND board_index_top_valid(Index) 
            AND NOT board_index_empty(pBoard, board_index_left(board_index_top(Index)))) 
        OR (board_index_left_valid(Index) 
            AND board_index_bottom_valid(Index) 
            AND NOT board_index_empty(pBoard, board_index_left(board_index_bottom(Index)))) 
        OR (board_index_right_valid(Index) 
            AND board_index_top_valid(Index) 
            AND NOT board_index_empty(pBoard, board_index_right(board_index_top(Index)))) 
        OR (board_index_right_valid(Index) 
            AND board_index_bottom_valid(Index) 
            AND NOT board_index_empty(pBoard, board_index_right(board_index_bottom(Index)))) 
    );
}

static tSize board_index_longest_path(tBoard *pBoard, tIndex Index, uint64_t Checked)
{
    tSize LeftLen = 0, RightLen = 0, TopLen = 0, BottomLen = 0;
    bool Player = board_index_player(pBoard, Index);
    BitSet64(&Checked, Index);

    if (board_index_left_valid(Index) 
        AND NOT board_index_empty(pBoard, board_index_left(Index)) 
        AND board_index_player(pBoard, board_index_left(Index)) == Player 
        AND NOT BitTest64(Checked, board_index_left(Index)))
    {
        LeftLen += board_index_longest_path(pBoard, board_index_left(Index), Checked);
    }
    if (board_index_right_valid(Index) 
        AND NOT board_index_empty(pBoard, board_index_right(Index)) 
        AND board_index_player(pBoard, board_index_right(Index)) == Player 
        AND NOT BitTest64(Checked, board_index_right(Index)))
    {
        RightLen += board_index_longest_path(pBoard, board_index_right(Index), Checked);
    }
    if (board_index_top_valid(Index)
        AND NOT board_index_empty(pBoard, board_index_top(Index)) 
        AND board_index_player(pBoard, board_index_top(Index)) == Player 
        AND NOT BitTest64(Checked, board_index_top(Index)))
    {
        TopLen += board_index_longest_path(pBoard, board_index_top(Index), Checked);
    }
    if (board_index_bottom_valid(Index)
        AND NOT board_index_empty(pBoard, board_index_bottom(Index)) 
        AND board_index_player(pBoard, board_index_bottom(Index)) == Player
        AND NOT BitTest64(Checked, board_index_bottom(Index)))
    {
        BottomLen += board_index_longest_path(pBoard, board_index_bottom(Index), Checked);
    }

    return 1 + max_size(max_size(max_size(LeftLen, RightLen), TopLen), BottomLen);
}

static tSize board_index_checked_path(tBoard *pBoard, tIndex Index, uint64_t Chkd, uint64_t *pChkOut)
{
    tSize PathLen, MaxPathLen = 0;
    uint64_t Checked, MaxChecked, ThisChecked = 0ULL;
    bool Player = board_index_player(pBoard, Index);
    BitSet64(&ThisChecked, Index);
    Chkd |= ThisChecked;
    MaxChecked = ThisChecked;

    if (board_index_left_valid(Index)
        AND NOT board_index_empty(pBoard, board_index_left(Index)) 
        AND board_index_player(pBoard, board_index_left(Index)) == Player 
        AND NOT BitTest64(Chkd, board_index_left(Index)))
    {
        Checked = ThisChecked;
        PathLen = board_index_checked_path(pBoard, board_index_left(Index), Chkd, &Checked);
        SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Checked, MaxChecked);
    }
    if (board_index_right_valid(Index)
        AND NOT board_index_empty(pBoard, board_index_right(Index)) 
        AND board_index_player(pBoard, board_index_right(Index)) == Player 
        AND NOT BitTest64(Chkd, board_index_right(Index)))
    {
        Checked = ThisChecked;
        PathLen = board_index_checked_path(pBoard, board_index_right(Index), Chkd, &Checked);
        SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Checked, MaxChecked);
    }
    if (board_index_top_valid(Index)
        AND NOT board_index_empty(pBoard, board_index_top(Index)) 
        AND board_index_player(pBoard, board_index_top(Index)) == Player  
        AND NOT BitTest64(Chkd, board_index_top(Index)))
    {
        Checked = ThisChecked;
        PathLen = board_index_checked_path(pBoard, board_index_top(Index), Chkd, &Checked);
        SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Checked, MaxChecked);
    }
    if (board_index_bottom_valid(Index)
        AND NOT board_index_empty(pBoard, board_index_bottom(Index)) 
        AND board_index_player(pBoard, board_index_bottom(Index)) == Player 
        AND NOT BitTest64(Chkd, board_index_bottom(Index)))
    {
        Checked = ThisChecked;
        PathLen = board_index_checked_path(pBoard, board_index_bottom(Index), Chkd, &Checked);
        SET_IF_GREATER_W_EXTRA(PathLen, MaxPathLen, Checked, MaxChecked);
    }
    
    *pChkOut |= MaxChecked;
    return 1 + MaxPathLen;
}

bool board_index_valid(tIndex Index)
{
    return Index >= 0 AND Index < ROWS*COLUMNS;
}

bool board_index_empty(tBoard *pBoard, tIndex Index)
{
    if (NOT board_index_valid(Index))
    {
        dbg_printf(DEBUG_WARN, "Invalid index\n");
    }

    return NOT BitTest64(pBoard->Valid, Index);
}

bool board_index_player(tBoard *pBoard, tIndex Index)
{
    if (board_index_empty(pBoard, Index))
    {
        dbg_printf(DEBUG_WARN, "Index empty\n");
    }

    return BitTest64(pBoard->Data, Index);
}

bool board_index_left_valid(tIndex Index)
{
    return Index % COLUMNS > 0;
}

bool board_index_right_valid(tIndex Index)
{
    return Index % COLUMNS < COLUMNS-1;
}

bool board_index_top_valid(tIndex Index)
{
    return Index >= COLUMNS;
}

bool board_index_bottom_valid(tIndex Index)
{
    return Index < ROWS*(COLUMNS-1);
}

tIndex board_index_left(tIndex Index)
{
    return Index-1;
}

tIndex board_index_right(tIndex Index)
{
    return Index+1;
}

tIndex board_index_top(tIndex Index)
{
    return Index-COLUMNS;
}

tIndex board_index_bottom(tIndex Index)
{
    return Index+COLUMNS;
}

static tSize max_size(tSize A, tSize B)
{
    return (A > B) ? A : B;
}
