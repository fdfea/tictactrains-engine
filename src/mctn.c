#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "board.h"
#include "mctn.h"
#include "mctnlist.h"
#include "types.h"
#include "util.h"

static uint32_t mctn_size(tMctn *pNode);
static float uct(tVisits ParentVisits, tVisits Visits, float Score);

void mctn_init(tMctn *pNode, tBoard *pBoard)
{
    board_copy(&pNode->State, pBoard);
    mctnlist_init(&pNode->Children);

    pNode->Visits = 0;
    pNode->Score = 0.0f;
}

void mctn_free(tMctn *pNode)
{
    mctnlist_free(&pNode->Children);
}

void mctn_copy(tMctn *pNode, tMctn *pN)
{
    *pNode = *pN;
}

void mctn_update(tMctn *pNode, float Score)
{
    pNode->Score += Score;
    pNode->Visits++;
}

void mctn_expand(tMctn *pNode, tBoard *pStates, tSize Size)
{
    mctnlist_expand(&pNode->Children, pStates, Size);
}

bool mctn_equals(tMctn *pNode, tMctn *pN)
{
    return board_equals(&pNode->State, &pN->State);
}

tMctn *mctn_random_child(tMctn *pNode, tRandom *pRandom)
{
    tMctn *pChild = NULL;
    tSize Size = mctnlist_size(&pNode->Children);

    if (Size > 0)
    {
        pChild = mctnlist_get(&pNode->Children, random_next(pRandom) % Size);
    }

    return pChild;
}

tMctn *mctn_most_visited_child(tMctn *pNode)
{
    tMctn *pChild, *pWinner = NULL;
    tVisits MaxVisits = 0;

    for (tIndex i = 0; i < mctnlist_size(&pNode->Children); ++i)
    {
        pChild = mctnlist_get(&pNode->Children, i);
        SET_IF_GREATER_EQ_W_EXTRA(pChild->Visits, MaxVisits, pChild, pWinner);
    }

    return pWinner;
}

tMctn *mctn_best_child_uct(tMctn *pNode)
{
    tMctn *pChild, *pWinner = NULL;
    float MaxUct = -FLT_MAX;

    for (tIndex i = 0; i < mctnlist_size(&pNode->Children); ++i)
    {
        pChild = mctnlist_get(&pNode->Children, i);
        SET_IF_GREATER_EQ_W_EXTRA(uct(pNode->Visits, pChild->Visits, pChild->Score), MaxUct, pChild, pWinner);
    }

    return pWinner;
}

char *mctn_string(tMctn *pNode)
{
    char *Str = emalloc(MCTN_STR_LEN * sizeof(char)), *pBegin = Str, *pId = NULL;
    tVisits Visits = pNode->Visits;

    Str += sprintf(Str, "Tree size: %d, Root score: %.2f/%d\n",
        mctn_size(pNode), pNode->Score, Visits);

    for (tIndex i = 0; i < mctnlist_size(&pNode->Children); ++i)
    {
        tMctn *pChild = mctnlist_get(&pNode->Children, i);
        pId = board_index_id(board_last_move_index(&pChild->State));

        float Eval = IF (pChild->Visits > 0) THEN pChild->Score/pChild->Visits ELSE 0.0f;
        
        Str += sprintf(Str, "%s: %0.2f @ %.2f/%d ** %d Nodes ** %3.3e UCT\n", 
            pId, Eval, pChild->Score, pChild->Visits, mctn_size(pChild), 
            UCT(Visits, pChild->Visits, pChild->Score));

        free(pId);
    }

    return pBegin;
}

static uint32_t mctn_size(tMctn *pNode)
{
    uint32_t Size = 1;

    for (tIndex i = 0; i < mctnlist_size(&pNode->Children); ++i)
    {
        Size += mctn_size(mctnlist_get(&pNode->Children, i));
    }

    return Size;
}

static float uct(tVisits ParentVisits, tVisits Visits, float Score)
{
    return IF (Visits == 0) THEN FLT_MAX ELSE (Score/Visits) + sqrtf(2*logf(ParentVisits)/Visits);
}
