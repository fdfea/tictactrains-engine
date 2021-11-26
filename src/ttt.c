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

static int ttt_load_moves(tTTT *pGame, tVector *Moves);

int main(void)
{
    int Res = 0;

    tTTT Game;
    tConfig Config;
    
    config_init(&Config);

    Res = config_load(&Config);
    if (Res < 0)
    {
        goto Error;
    }

    Res = ttt_init(&Game, &Config);
    if (Res < 0)
    {
        goto Error;
    }

    bool ComputerPlaying = Config.ComputerPlaying;
    bool ComputerPlayer = Config.ComputerPlayer;

    config_free(&Config);

    char *pBoardStr = NULL;
    char *pMovesStr = NULL;
#ifdef STATS
    char *pMctsStr = NULL;
#endif
    int *pMoves = NULL;
    int MovesSize;

    pBoardStr = board_string(&Game.Board);
    printf("%s\n\n", pBoardStr);
    free(pBoardStr);
    pBoardStr = NULL;

    char (*pId)[BOARD_ID_STR_LEN] = malloc(sizeof(char)*BOARD_ID_STR_LEN);
    if (pId IS NULL)
    {
        Res = -ENOMEM;
        dbg_printf(DEBUG_ERROR, "No memory available\n");
        goto Error;
    }

    while (NOT board_finished(&Game.Board))
    {
        int Index;
        bool Player = ttt_get_player(&Game);

        if (ComputerPlaying AND Player == ComputerPlayer) 
        {
            Index = ttt_get_ai_move(&Game);
            if (Index < 0)
            {
                Res = Index;
                goto Error;
            }
        } 
        else
        {
            uint64_t Policy = rules_policy(&Game.Rules, &Game.Board);
            uint64_t Indices = board_valid_indices(&Game.Board, Policy, false);

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
                    
                    //add parsing for special commands
                    //show simulations or lines, quit, etc.

                    if (board_id_valid(pId)) 
                    {
                        Index = board_id_index(pId);

                        if (BitTest64(Indices, (tIndex) Index))
                        {
                            break;
                        }
                    }
                }
            }
        }

#ifdef STATS
        if (ComputerPlaying)
        {
            pMctsStr = mctn_string(Game.Mcts.pRoot);
            printf("BEFORE SHIFT\n%s\n", pMctsStr);
            free(pMctsStr);
            pMctsStr = NULL;

            float Eval = mcts_get_eval(&Game.Mcts);
            if (Eval > -FLT_MAX) printf("Eval: %.2f\n\n", Eval);
        }

#endif

        ttt_give_move(&Game, Index);

#ifdef STATS
        if (ComputerPlaying)
        {
            pMctsStr = mctn_string(Game.Mcts.pRoot);
            printf("AFTER SHIFT\n%s\n", pMctsStr);
            free(pMctsStr);
            pMctsStr = NULL;
        }
#endif

        pMoves = ttt_get_moves(&Game, &MovesSize);
        pMovesStr = rules_moves_string(&Game.Rules, pMoves, MovesSize);
        free(pMoves);
        pMoves = NULL;
        printf("%s\n", pMovesStr);
        free(pMovesStr);
        pMovesStr = NULL;
        pBoardStr = board_string(&Game.Board);
        printf("%s\n\n", pBoardStr);
        free(pBoardStr);
        pBoardStr = NULL;
    }

    int Score = ttt_get_score(&Game);
    printf("Score: %d\n", Score);

    free(pId);
    ttt_free(&Game);

Error:
    return Res;
}

int ttt_init(tTTT *pGame, tConfig *pConfig)
{
    int Res = 0;

    board_init(&pGame->Board);
    rules_init(&pGame->Rules, &pConfig->RulesConfig);
    memset(&pGame->Moves, 0, sizeof(pGame->Moves));

    Res = mcts_init(&pGame->Mcts, &pGame->Rules, &pGame->Board, &pConfig->MctsConfig);
    if (Res < 0)
    {
        goto Error;
    }

    Res = ttt_load_moves(pGame, &pConfig->StartPosition);
    if (Res < 0)
    {
        goto Error;
    }

Error:
    return Res;
}

void ttt_free(tTTT *pGame)
{
    mcts_free(&pGame->Mcts);
}

static int ttt_load_moves(tTTT *pGame, tVector *pMoves)
{
    int Res = 0;

    tBoard BoardCopy;
    board_init(&BoardCopy);
    board_copy(&BoardCopy, &pGame->Board);

    for (tIndex i = 0; i < vector_size(pMoves); ++i)
    {
        tIndex Move = *((tIndex *) vector_get(pMoves, i));
        Res = ttt_give_move(pGame, Move);
        if (Res < 0)
        {
            dbg_printf(DEBUG_ERROR, "Failed to load moves\n");
            board_copy(&pGame->Board, &BoardCopy);
            goto Error;
        }
    }

Error:
    return Res;
}

int ttt_get_ai_move(tTTT *pGame)
{
    int Res, Index;
    uint64_t Policy = rules_policy(&pGame->Rules, &pGame->Board);
    uint64_t Indices = board_valid_indices(&pGame->Board, Policy, false);

    if (BitTest64(Indices, 24)) 
    {
        Index = 24;
    } 
    else 
    {
        mcts_simulate(&pGame->Mcts);
        tBoard *pNextState = mcts_get_state(&pGame->Mcts);
        if (pNextState IS NULL)
        {
            Res = -ENODATA;
            goto Error;
        }

        Index = board_last_move_index(pNextState);
    }
    goto Success;

Error:
    Index = Res;
Success:
    return Index;
}

int ttt_give_move(tTTT *pGame, int Index)
{
    int Res = 0;
    uint64_t Policy = rules_policy(&pGame->Rules, &pGame->Board);
    uint64_t Indices = board_valid_indices(&pGame->Board, Policy, false);

    if (NOT board_index_valid(Index) 
        OR NOT BitTest64(Indices, Index))
    {
        Res = -EINVAL;
        dbg_printf(DEBUG_ERROR, "Invalid move attempted\n");
        goto Error;
    }

    bool Player = rules_player(&pGame->Rules, &pGame->Board);
    pGame->Moves[board_move(&pGame->Board)] = (tIndex) Index;
    board_make_move(&pGame->Board, (tIndex) Index, Player);
    mcts_give_state(&pGame->Mcts, &pGame->Board);

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
    return (int) board_score(&pGame->Board, SCORING_ALGORITHM_OPTIMAL);
}

int *ttt_get_moves(tTTT *pGame, int *pSize)
{
    int *pMoves = NULL;
    *pSize = (int) board_move(&pGame->Board);

    pMoves = malloc(*pSize*sizeof(int));
    if (pMoves IS NULL)
    {
        dbg_printf(DEBUG_ERROR, "No memory available\n");
        *pSize = 0;
        goto Error;
    }

    for (tIndex i = 0; i < *pSize; ++i)
    {
        pMoves[i] = (int) pGame->Moves[i];
    }

Error:
    return pMoves;
}
