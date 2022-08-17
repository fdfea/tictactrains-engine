#include <errno.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "board.h"
#include "debug.h"
#include "mctn.h"
#include "mctnlist.h"
#include "mcts.h"
#include "rules.h"
#include "random.h"
#include "types.h"
#include "util.h"

#define BOARD_WIN   1.0f
#define BOARD_DRAW  0.5f
#define BOARD_LOSS  0.0f

#define BOARD_WIN_BASE      0.90f
#define BOARD_WIN_BONUS     0.025f
#define BOARD_LOSS_BASE     0.10f
#define BOARD_LOSS_PENALTY  0.025f

static void mcts_expand_node(tMcts *pMcts, tMctn *pNode);
static float mcts_simulation(tMcts *pMcts, tMctn *pNode);
static float mcts_simulate_playout(tMcts *pMcts, tBoard *pState);
static float mcts_weight_score(tScore Score);

#ifdef TIMED
static double time_diff_ms(struct timespec *pBegin, struct timespec *pEnd);
#endif

void mcts_init(tMcts *pMcts, tRules *pRules, tBoard *pState, tMctsConfig *pConfig)
{
    pMcts->pRoot = emalloc(sizeof(tMctn));
    pMcts->pRules = pRules;
    pMcts->Player = rules_player(pRules, pState);
    pMcts->Config = *pConfig;

    mctn_init(pMcts->pRoot, pState);
    rand_init(&pMcts->Rand);
}

void mcts_config_init(tMctsConfig *pConfig)
{
    pConfig->Simulations = 1000;
    pConfig->ScoringAlgorithm = SCORING_ALGORITHM_OPTIMAL;
    pConfig->SearchOnlyNeighbors = true;
}

void mcts_free(tMcts *pMcts)
{
    mctn_free(pMcts->pRoot);
    free(pMcts->pRoot);
}

void mcts_simulate(tMcts *pMcts)
{
    //set simulations to pRoot visits?
    tVisits Count = 0, Simulations = pMcts->Config.Simulations;

#ifdef TIMED
    struct timespec Begin, End;
    clock_gettime(CLOCK_REALTIME, &Begin);
#endif

    while (Count < Simulations AND pMcts->pRoot->Visits < TVISITS_MAX) 
    {
        mcts_simulation(pMcts, pMcts->pRoot);
        Count++;
    }

#ifdef TIMED
    clock_gettime(CLOCK_REALTIME, &End);
    double Diff = time_diff_ms(&Begin, &End);
    printf("Simulations: %d, Time elapsed: %.3lf ms\n", Count, Diff);
#endif
}

//should this return a pointer?
tBoard *mcts_get_state(tMcts *pMcts)
{
    tBoard *pBoard = NULL;

    if (NOT board_finished(&pMcts->pRoot->State)
        AND NOT mctnlist_empty(&pMcts->pRoot->Children))
    {
        pBoard = &mctn_most_visited_child(pMcts->pRoot, &pMcts->Rand)->State;
    }
    else
    {
        dbg_printf(DEBUG_LEVEL_ERROR, "No state available");
    }

    return pBoard;
}

void mcts_give_state(tMcts *pMcts, tBoard *pState)
{
    tRules *pRules = pMcts->pRules;
    tMctn Tmp, *pTmp = &Tmp;
    mctn_init(pTmp, pState);

    //should keep stats from previous node
    if (NOT board_finished(&pMcts->pRoot->State))
    {
        mctnlist_delete(&pMcts->pRoot->Children, pTmp);
        mctn_free(pMcts->pRoot);

        pMcts->pRoot = pTmp;
        pMcts->Player = rules_player(pRules, &pMcts->pRoot->State);
    }
}

float mcts_evaluate(tMcts *pMcts)
{
    float Eval = 0.0f;

    if (pMcts->pRoot->Visits > 0)
    {
        float Score = pMcts->pRoot->Score / pMcts->pRoot->Visits;
        Eval = IF (pMcts->Player) THEN (1.0f - Score) ELSE Score;
        Eval = 2.0f * Eval - 1.0f;
    }

    return Eval;
}

static void mcts_expand_node(tMcts *pMcts, tMctn *pNode)
{
    tSize Size;
    tBoard* pStates = rules_next_states(pMcts->pRules, &pNode->State, &Size, pMcts->Config.SearchOnlyNeighbors);

    mctn_expand(pNode, pStates, Size);
    mctnlist_shuffle(&pNode->Children, &pMcts->Rand);
    free(pStates);
}

static float mcts_simulation(tMcts *pMcts, tMctn *pNode)
{
    float Res;
    tRules *pRules = pMcts->pRules;
    tBoard *pState = &pNode->State;
    tRandom *pRand = &pMcts->Rand;

    if (mctn_list_empty(&pNode->Children))
    {
        if (NOT board_finished(pState))
        {
            mcts_expand_node(pMcts, pNode);
            Res = mcts_simulate_playout(pMcts, &mctn_random_child(pNode, pRand)->State);
        }
        else 
        {
            Res = mcts_simulate_playout(pMcts, pState);
        }
    }
    else
    {
        Res = mcts_simulation(pMcts, mctn_best_child_uct(pNode));
    }

    if (NOT board_finished(pState))
    {
        bool Player = rules_prev_player(pRules, pState);
        float Score = IF (Player == pMcts->Player) THEN Res ELSE 1.0f - Res;

        mctn_update(pNode, Score);
    }

    return Res;
}

static float mcts_simulate_playout(tMcts *pMcts, tBoard *pState)
{
    float Res, Score;
    tBoard Board;

    board_copy(&Board, pState);
    rules_simulate_playout(pMcts->pRules, &Board, &pMcts->Rand, true);
    
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
    return ((pEnd->tv_sec * 1.0e3) + (pEnd->tv_nsec / 1.0e6)) - 
        ((pBegin->tv_sec * 1.0e3) + (pBegin->tv_nsec / 1.0e6));
}
#endif
