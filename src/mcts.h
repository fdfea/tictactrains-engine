#ifndef __MCTS_H__
#define __MCTS_H__

#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "board_pred.h"
#include "mctn.h"
#include "rules.h"
#include "random.h"

typedef enum PredictionPolicy 
{
    PREDICTION_POLICY_NEVER        = 1,
    PREDICTION_POLICY_ALWAYS       = 2,
    PREDICTION_POLICY_LONGPATHS    = 3,
}
ePredictionPolicy;

typedef struct MctsConfig 
{
    tVisits Simulations;
    eScoringAlgorithm ScoringAlgorithm;
    ePredictionPolicy PredictionPolicy;
    ePredictionStrategy PredictionStrategy;
}
tMctsConfig;

typedef struct Mcts
{
    tMctn *pRoot;
    tRules *pRules;
    tRandom Rand;
    tMctsConfig Config;
    bool Player;
    bool Predict;
} 
tMcts;

int mcts_init(tMcts *pMcts, tRules *pRules, tBoard *pState, tMctsConfig *pConfig);
void mcts_config_init(tMctsConfig *pConfig);
void mcts_free(tMcts *pMcts);
void mcts_simulate(tMcts *pMcts);
tBoard *mcts_get_state(tMcts *pMcts);
void mcts_give_state(tMcts *pMcts, tBoard *pState);
float mcts_get_eval(tMcts *pMcts);

#endif
