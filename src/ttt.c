#include <errno.h>
#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitutil.h"
#include "board.h"
#include "config.h"
#include "debug.h"
#include "mctn.h"
#include "mcts.h"
#include "rules.h"
#include "ttt.h"
#include "types.h"
#include "util.h"
#include "vector.h"

static int ttt_load_moves(tTTT *pGame, tVector *pMoves);

int main(void)
{
    int Res = 0;

    tTTT Game;
    tConfig Config;
    
    Res = config_init(&Config);
    if (Res < 0)
    {
        goto Error;
    }

    Res = config_load(&Config);
    if (Res < 0)
    {
        goto FreeConfig;
    }

    Res = ttt_init(&Game, &Config);
    if (Res < 0)
    {
        goto FreeConfig;
    }

    char *pBoardStr, *pMovesStr;
#ifdef STATS
    char *pMctsStr;
#endif
    int *pMoves;
    int MovesSize;

    pBoardStr = board_string(&Game.Board);
    printf("%s\n\n", pBoardStr);
    free(pBoardStr);

    while (NOT board_finished(&Game.Board))
    {
        int Index;
        bool Player = ttt_get_player(&Game);

        Res = IF (Config.ComputerPlaying AND Player == Config.ComputerPlayer)
            THEN ttt_get_ai_move(&Game)
            ELSE ttt_get_player_move(&Game, Config.ComputerPlaying);

        if (Res < 0)
        {
            goto FreeGame;
        }

        Index = Res;

#ifdef STATS
        if (ComputerPlaying && Game.Mcts.pRoot->Visits > 0)
        {
            pMctsStr = mctn_string(Game.Mcts.pRoot);
            printf("BEFORE SHIFT\n%s\n", pMctsStr);
            free(pMctsStr);

            float Eval = mcts_evaluate(&Game.Mcts);
            if (Eval > -FLT_MAX) printf("Eval: %.2f\n\n", Eval);
        }
#endif

        ttt_give_move(&Game, Index);

#ifdef STATS
        if (ComputerPlaying && Game.Mcts.pRoot->Visits > 0)
        {
            pMctsStr = mctn_string(Game.Mcts.pRoot);
            printf("AFTER SHIFT\n%s\n", pMctsStr);
            free(pMctsStr);

            float Eval = mcts_evaluate(&Game.Mcts);
            if (Eval > -FLT_MAX) printf("Eval: %.2f\n\n", Eval);
        }
#endif

        pMoves = ttt_get_moves(&Game, &MovesSize);
        pMovesStr = rules_moves_string(&Game.Rules, pMoves, MovesSize);
        pBoardStr = board_string(&Game.Board);

        printf("%s\n", pMovesStr);
        printf("%s\n\n", pBoardStr);

        free(pMoves);
        free(pMovesStr);
        free(pBoardStr);
    }

    int Score = ttt_get_score(&Game);
    printf("Score: %d\n", Score);

FreeGame:
    ttt_free(&Game);

FreeConfig:
    config_free(&Config);

Error:
    return Res;
}

int ttt_init(tTTT *pGame, tConfig *pConfig)
{
    int Res = 0;

    board_init(&pGame->Board);
    rules_init(&pGame->Rules, &pConfig->RulesConfig);
    mcts_init(&pGame->Mcts, &pGame->Rules, &pGame->Board, &pConfig->MctsConfig);
    memset(&pGame->Moves, 0, sizeof(pGame->Moves));

    Res = ttt_load_moves(pGame, &pConfig->StartingMoves);
    if (Res < 0)
    {
        goto Error;
    }

    goto Success;

Error:
    mcts_free(&pGame->Mcts);

Success:
    return Res;
}

void ttt_free(tTTT *pGame)
{
    mcts_free(&pGame->Mcts);
}

int ttt_get_player_move(tTTT *pGame, bool ComputerPlaying)
{
    int Index;
    bool Player = ttt_get_player(pGame);
    uint64_t Indices = rules_indices(&pGame->Rules, &pGame->Board, true);

    char (*pId)[BOARD_ID_STR_LEN] = emalloc(sizeof(char)*BOARD_ID_STR_LEN);

    while (true)
    {
        if (ComputerPlaying)
        {
            printf("Enter move: ");
        }
        else
        {
            printf("Enter move (Player %d): ", (Player ? 1 : 2));
        }

        if (fgets(*pId, BOARD_ID_STR_LEN, stdin) ISNOT NULL)
        {
            int c; 
            while ((c = getchar()) != '\n' AND c != EOF);

            if (board_id_valid(pId)) 
            {
                Index = board_id_index(pId);

                if (BitTest64(Indices, Index))
                {
                    break;
                }
            }
        }
    }

    free(pId);

    return Index;
}

int ttt_get_ai_move(tTTT *pGame)
{
    int Res, Index;
    uint64_t Indices = rules_indices(&pGame->Rules, &pGame->Board, false);

    if (BitTest64(Indices, 24)) 
    {
        Index = 24;
    } 
    else 
    {
        mcts_simulate(&pGame->Mcts);
        tBoard *pState = mcts_get_state(&pGame->Mcts);
        if (pState IS NULL)
        {
            Res = -ENODATA;
            goto Error;
        }

        Index = board_last_move_index(pState);
    }

    Res = Index;

Error:
    return Res;
}

int ttt_give_move(tTTT *pGame, int Index)
{
    int Res = 0;
    uint64_t Indices = rules_indices(&pGame->Rules, &pGame->Board, false);

    if (NOT board_index_valid(Index) 
        OR NOT BitTest64(Indices, Index))
    {
        Res = -EINVAL;
        dbg_printf(DEBUG_LEVEL_ERROR, "Invalid move attempted\n");
        goto Error;
    }

    board_advance(&pGame->Board, Index, rules_player(&pGame->Rules, &pGame->Board));
    mcts_give_state(&pGame->Mcts, &pGame->Board);

    pGame->Moves[board_move(&pGame->Board)] = Index;

Error:
    return Res;
}

bool ttt_get_player(tTTT *pGame)
{
    return rules_player(&pGame->Rules, &pGame->Board);
}

bool ttt_is_finished(tTTT *pGame)
{
    return board_finished(&pGame->Board);
}

int ttt_get_score(tTTT *pGame)
{
    return board_score(&pGame->Board, SCORING_ALGORITHM_OPTIMAL);
}

int *ttt_get_moves(tTTT *pGame, int *pSize)
{
    *pSize = board_move(&pGame->Board);

    int *pMoves = emalloc(*pSize * sizeof(int));

    for (tIndex i = 0; i < *pSize; ++i)
    {
        pMoves[i] = pGame->Moves[i];
    }

    return pMoves;
}

static int ttt_load_moves(tTTT *pGame, tVector *pMoves)
{
    int Res = 0;

    tBoard BoardCopy;
    board_init(&BoardCopy);
    board_copy(&BoardCopy, &pGame->Board);

    for (tIndex i = 0; i < vector_size(pMoves); ++i)
    {
        Res = ttt_give_move(pGame, *((tIndex *) vector_get(pMoves, i)));
        if (Res < 0)
        {
            board_copy(&pGame->Board, &BoardCopy);
            dbg_printf(DEBUG_LEVEL_ERROR, "Failed to load moves\n");
            goto Error;
        }
    }

Error:
    return Res;
}
