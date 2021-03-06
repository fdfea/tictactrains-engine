#include <errno.h>
#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TIMED
#include <time.h>
#endif

#include "board.h"
#include "debug.h"
#include "mctn.h"
#include "mctn_list.h"
#include "mcts.h"
#include "rules.h"
#include "random.h"
#include "types.h"
#include "util.h"

static const float BOARD_WIN    = 1.0f;
static const float BOARD_DRAW   = 0.5f;
static const float BOARD_LOSS   = 0.0f;

static const float BOARD_WIN_BASE       = 0.90f;
static const float BOARD_WIN_BONUS      = 0.025f;
static const float BOARD_LOSS_BASE      = 0.10f;
static const float BOARD_LOSS_PENALTY   = 0.025f;

static int mcts_expand_node(tMcts *pMcts, tMctn *pNode);
static float mcts_simulation(tMcts *pMcts, tMctn *pNode);
static float mcts_simulate_playout(tMcts *pMcts, tBoard *pState);
static float mcts_weight_score(tScore Score);

#ifdef TIMED
static double time_diff_ms(struct timespec *pBegin, struct timespec *pEnd);
#endif

int mcts_init(tMcts *pMcts, tRules *pRules, tBoard *pState, tMctsConfig *pConfig)
{
    int Res = 0;

    pMcts->pRoot = malloc(sizeof(tMctn));
    if (pMcts->pRoot IS NULL)
    {
        Res = -ENOMEM;
        dbg_printf(DEBUG_LEVEL_ERROR, "No memory available\n");
        goto Error;
    }

    mctn_init(pMcts->pRoot, pState);
    pMcts->pRules = pRules;
    rand_init(&pMcts->Rand);
    pMcts->Player = rules_player(pRules, pState);
    pMcts->Config = *pConfig;

Error: 
    return Res;
}

void mcts_config_init(tMctsConfig *pConfig)
{
    pConfig->Simulations = 1000;
    pConfig->ScoringAlgorithm = SCORING_ALGORITHM_OPTIMAL;
}

void mcts_free(tMcts *pMcts)
{
    mctn_free(pMcts->pRoot);
    free(pMcts->pRoot);
}

void mcts_simulate(tMcts *pMcts)
{
    tVisits Simulations = pMcts->Config.Simulations, Count = 0;

#ifdef TIMED
    struct timespec Begin, End;
    clock_gettime(CLOCK_REALTIME, &Begin);
#endif

    while (Count < Simulations AND pMcts->pRoot->Visits < TVISITS_MAX) 
    {
        (void) mcts_simulation(pMcts, pMcts->pRoot);
        Count++;
    }

#ifdef TIMED
    clock_gettime(CLOCK_REALTIME, &End);
    double Diff = time_diff_ms(&Begin, &End);
    printf("Simulations: %d, Time elapsed: %.3lf ms\n", Count, Diff);
#endif
}

tBoard *mcts_get_state(tMcts *pMcts)
{
    tBoard *pBoard = NULL;

    if (NOT board_finished(&pMcts->pRoot->State)
        AND NOT mctn_list_empty(&pMcts->pRoot->Children))
    {
        tMctn *pBestChild = mctn_most_visited_child(pMcts->pRoot);
        pBoard = &pBestChild->State;
    }
    else
    {
        dbg_printf(DEBUG_LEVEL_ERROR, "No state available\n");
    }

    return pBoard;
}

void mcts_give_state(tMcts *pMcts, tBoard *pState)
{
    tRules *pRules = pMcts->pRules;
    tMctn NewRoot;
    mctn_init(&NewRoot, pState);

    if (NOT board_finished(&pMcts->pRoot->State) 
        AND NOT mctn_list_empty(&pMcts->pRoot->Children))
    {
        for (tIndex i = 0; i < mctn_list_size(&pMcts->pRoot->Children); ++i)
        {
            tMctn *pChild = mctn_list_get(&pMcts->pRoot->Children, i);

            if (mctn_equals(&NewRoot, pChild))
            {
                mctn_free(&NewRoot);
                NewRoot = *pChild;
                (void) mctn_list_delete(&pMcts->pRoot->Children, i);
                break;
            }
        }
    }

    mctn_free(pMcts->pRoot);
    *pMcts->pRoot = NewRoot;

    if (NOT board_finished(&pMcts->pRoot->State))
    {
        pMcts->Player = rules_player(pRules, &pMcts->pRoot->State);
    }
}

float mcts_evaluate(tMcts *pMcts)
{
    float Eval = 0.0f;

    if (pMcts->pRoot->Visits > 0)
    {
        float Score = pMcts->pRoot->Score / (float) pMcts->pRoot->Visits;
        Eval = IF (pMcts->Player) THEN (1.0f - Score) ELSE Score;
        Eval = 2.0f * Eval - 1.0f;
    }

    return Eval;
}

static int mcts_expand_node(tMcts *pMcts, tMctn *pNode)
{
    int Res = 0;

    tVector NextStates;
    Res = vector_init(&NextStates);
    if (Res < 0)
    {
        goto Error;
    }

    rules_next_states(pMcts->pRules, &pNode->State, &NextStates);

    Res = mctn_expand(pNode, &NextStates);
    if (Res < 0)
    {
        goto Error;
    }

    for (tIndex i = 0; i < vector_size(&NextStates); i++) 
    {
        tBoard *pBoard = (tBoard *) vector_get(&NextStates, i);
        free(pBoard);
    }
    vector_free(&NextStates);
    mctn_list_shuffle(&pNode->Children, &pMcts->Rand);

Error:
    return Res;
}

static float mcts_simulation(tMcts *pMcts, tMctn *pNode)
{
    float Res = 0.0f;
    tRules *pRules = pMcts->pRules;
    tBoard *pState = &pNode->State;
    tRandom *pRand = &pMcts->Rand;

    if (mctn_list_empty(&pNode->Children))
    {
        if (NOT board_finished(pState))
        {
            if (mcts_expand_node(pMcts, pNode) < 0)
            {
                goto Error;
            }

            tMctn *pChild = mctn_random_child(pNode, pRand);
            Res = mcts_simulate_playout(pMcts, &pChild->State);
        }
        else 
        {
            Res = mcts_simulate_playout(pMcts, pState);
        }
    }
    else
    {
        tMctn *pBestChild = mctn_best_child_uct(pNode);
        Res = mcts_simulation(pMcts, pBestChild);
    }

    if (NOT board_finished(pState))
    {
        bool StatePlayer = rules_prev_player(pRules, pState);
        float Score = IF (StatePlayer == pMcts->Player) THEN Res ELSE (1.0f - Res);

        mctn_update(pNode, Score);
    }

Error:
    return Res;
}

static float mcts_simulate_playout(tMcts *pMcts, tBoard *pState)
{
    float Res, Score;
    tBoard Board;
    board_copy(&Board, pState);

    rules_simulate_playout(pMcts->pRules, &Board, &pMcts->Rand);
    
    Score = mcts_weight_score(board_score(&Board, pMcts->Config.ScoringAlgorithm));

    Res = IF (pMcts->Player) THEN Score ELSE (1.0f - Score);

    return Res;
}

static float mcts_weight_score(tScore Score)
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

#ifdef TIMED
static double time_diff_ms(struct timespec *pBegin, struct timespec *pEnd)
{
    return (((double) pEnd->tv_sec * 1e3) + ((double) pEnd->tv_nsec / 1e6)) - 
        (((double) pBegin->tv_sec * 1e3) + ((double) pBegin->tv_nsec / 1e6));
}
#endif
