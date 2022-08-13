#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "mcts.h"
#include "rules.h"
#include "vector.h"

typedef struct Config
{
    bool ComputerPlaying;
    bool ComputerPlayer;
    tRulesConfig RulesConfig;
    tMctsConfig MctsConfig;
    tVector StartingMoves;
} 
tConfig;

int config_init(tConfig *pConfig);
void config_free(tConfig *pConfig);
int config_load(tConfig *pConfig);

#endif
