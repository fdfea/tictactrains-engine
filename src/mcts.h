#ifndef __MCTS_H__
#define __MCTS_H__

#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "mctn.h"
#include "rules.h"
#include "random.h"

typedef struct MctsConfig 
{
    tVisits Simulations;
    eScoringAlgorithm ScoringAlgorithm;
}
tMctsConfig;

typedef struct Mcts
{
    tMctn *pRoot;
    tRules *pRules;
    tRandom Rand;
    tMctsConfig Config;
    bool Player;
} 
tMcts;

int mcts_init(tMcts *pMcts, tRules *pRules, tBoard *pState, tMctsConfig *pConfig);
void mcts_config_init(tMctsConfig *pConfig);
void mcts_free(tMcts *pMcts);
void mcts_simulate(tMcts *pMcts);
tBoard *mcts_get_state(tMcts *pMcts);
void mcts_give_state(tMcts *pMcts, tBoard *pState);
float mcts_evaluate(tMcts *pMcts);

#endif
