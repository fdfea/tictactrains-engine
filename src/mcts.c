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
#include "board_pred.h"
#include "debug.h"
#include "mctn.h"
#include "mctn_list.h"
#include "mcts.h"
#include "rules.h"
#include "random.h"
#include "types.h"
#include "util.h"

#define PREDICTION_PATH_LENGTH  11
#define PREDICTION_MOVE_CUTOFF  44

static float mcts_simulation(tMcts *pMcts, tMctn *pNode);
static int mcts_expand_node(tMcts *pMcts, tMctn *pNode);
static float mcts_simulate_playout(tMcts *pMcts, tBoard *pState);
static void mcts_check_prediction_cond(tMcts *pMcts);

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
        dbg_printf(DEBUG_ERROR, "No memory available\n");
        goto Error;
    }

    mctn_init(pMcts->pRoot, pState);
    pMcts->pRules = pRules;
    rand_init(&pMcts->Rand);
    pMcts->Player = rules_player(pRules, pState);
    pMcts->Config = *pConfig;

    switch (pMcts->Config.PredictionPolicy)
    {
        case PREDICTION_POLICY_ALWAYS:
        {
            pMcts->Predict = true;
            break;
        }
        case PREDICTION_POLICY_LONGPATHS:
        case PREDICTION_POLICY_NEVER:
        {
            pMcts->Predict = false;
            break;
        }
        default: 
        {
            dbg_printf(DEBUG_WARN, "Invalid prediction policy\n");
            break;
        }
    }

Error: 
    return Res;
}

void mcts_config_init(tMctsConfig *pConfig)
{
    pConfig->Simulations = 10000;
    pConfig->ScoringAlgorithm = SCORING_ALGORITHM_OPTIMAL;
    pConfig->PredictionPolicy = PREDICTION_POLICY_NEVER;
    pConfig->PredictionStrategy = PREDICTION_STRATEGY_NBR_LINREG;
}

void mcts_free(tMcts *pMcts)
{
    mctn_free(pMcts->pRoot);
    free(pMcts->pRoot);
    pMcts->pRoot = NULL;
}

void mcts_simulate(tMcts *pMcts)
{
    tVisits Simulations = pMcts->Config.Simulations, Count = 0;

    mcts_check_prediction_cond(pMcts);

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
        dbg_printf(DEBUG_ERROR, "No state available\n");
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

float mcts_get_eval(tMcts *pMcts)
{
    if (pMcts->pRoot->Visits <= 0.0f) return -FLT_MAX;
    float Eval = pMcts->pRoot->Score / (float) pMcts->pRoot->Visits;
    return (pMcts->Player) ? 1.0f - Eval : Eval;
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
        float Score = (StatePlayer == pMcts->Player) 
            ? Res : 1.0f - Res;

        mctn_update(pNode, Score);
    }

Error:
    return Res;
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

static float mcts_simulate_playout(tMcts *pMcts, tBoard *pState)
{
    float Res, Score;
    tBoard Board;
    board_copy(&Board, pState);

    rules_simulate_playout(pMcts->pRules, &Board, &pMcts->Rand);
    
    Score = (pMcts->Predict) 
        ? board_predict_score(&Board, pMcts->Config.PredictionStrategy)
        : board_fscore(board_score(&Board, pMcts->Config.ScoringAlgorithm));

    Res = (pMcts->Player) ? Score : 1.0f - Score;

    return Res;
}

static void mcts_check_prediction_cond(tMcts *pMcts)
{
    if (pMcts->Config.PredictionPolicy == PREDICTION_POLICY_ALWAYS 
        OR pMcts->Config.PredictionPolicy == PREDICTION_POLICY_NEVER)
    {
        return;
    }

    if (board_move(&pMcts->pRoot->State) >= PREDICTION_MOVE_CUTOFF)
    {
        if (pMcts->Predict)
        {
            pMcts->Predict = false;
        }
    }
    else if (NOT pMcts->Predict AND pMcts->Config.PredictionPolicy == PREDICTION_POLICY_LONGPATHS)
    {
        tSize LongestPath = board_longest_path(&pMcts->pRoot->State);

        if (LongestPath >= PREDICTION_PATH_LENGTH)
        {
            pMcts->Predict = true;
        }
    }
}

#ifdef TIMED
static double time_diff_ms(struct timespec *pBegin, struct timespec *pEnd)
{
    return (((double) pEnd->tv_sec * 1e3) + ((double) pEnd->tv_nsec / 1e6)) - 
        (((double) pBegin->tv_sec * 1e3) + ((double) pBegin->tv_nsec / 1e6));
}
#endif
