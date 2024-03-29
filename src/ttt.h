#ifndef __TTT_H__
#define __TTT_H__

#include <stdbool.h>

#include "board.h"
#include "config.h"
#include "mcts.h"
#include "rules.h"
#include "types.h"

typedef struct TTT
{
    tBoard Board;
    tRules Rules;
    tMcts Mcts;
    tIndex Moves[ROWS*COLUMNS];
}
tTTT;

int ttt_init(tTTT *pGame, tConfig *pConfig);
void ttt_free(tTTT *pGame);
int ttt_get_ai_move(tTTT *pGame);
int ttt_give_move(tTTT *pGame, int Index);
bool ttt_get_player(tTTT *pGame);
bool ttt_finished(tTTT *pGame);
int ttt_get_score(tTTT *pGame);
int *ttt_get_moves(tTTT *pGame, int *pSize);

#endif
