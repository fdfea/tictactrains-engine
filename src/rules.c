#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitutil.h"
#include "debug.h"
#include "random.h"
#include "rules.h"
#include "types.h"
#include "util.h"

#define RULES_PLAYER_MASK   0x8000000000000000ULL

static uint64_t rules_policy(tRules *pRules, tBoard *pBoard);
static bool rules_index_player(tRules *pRules, tIndex Index);
static void rules_by_type(tRules *pRules, eRulesType RulesType);

void rules_init(tRules *pRules, tRulesConfig *pConfig)
{
    rules_by_type(pRules, pConfig->RulesType);
}

void rules_config_init(tRulesConfig *pConfig)
{
    pConfig->RulesType = RULES_CLASSICAL;
}

uint64_t rules_indices(tRules *pRules, tBoard *pBoard)
{
    return rules_policy(pRules, pBoard) & board_empty_indices(pBoard, true);
}

bool rules_player(tRules *pRules, tBoard *pBoard)
{
    return NOT BitEmpty64(rules_policy(pRules, pBoard) & RULES_PLAYER_MASK);
}

bool rules_prev_player(tRules *pRules, tBoard *pBoard)
{
    bool Player = false;
    tSize Move = board_move(pBoard);

    if (Move > 0)
    {
        Player = NOT BitEmpty64(pRules->MovePolicies[Move-1] & RULES_PLAYER_MASK);
    }

    return Player;
}

void rules_next_states(tRules *pRules, tBoard *pBoard, tVector *pVector)
{
    uint64_t Indices = rules_indices(pRules, pBoard);
    bool Player = rules_player(pRules, pBoard);

    while (NOT BitEmpty64(Indices)) 
    {
        tIndex Index = BitLzCount64(Indices);
        tBoard *pBoardCopy = emalloc(sizeof(tBoard));

        board_copy(pBoardCopy, pBoard);
        board_advance(pBoardCopy, Index, Player);
        vector_add(pVector, pBoardCopy);

        BitResetLsb64(&Indices, Index);
    }
}

void rules_simulate_playout(tRules *pRules, tBoard *pBoard, tRandom *pRand)
{
    while (NOT board_finished(pBoard))
    {
        uint64_t Indices = rules_indices(pRules, pBoard);
        tIndex Index = BitScanRandom64(Indices, pRand);

        board_advance(pBoard, Index, rules_player(pRules, pBoard));
    }
}

char *rules_moves_string(tRules *pRules, int *pMoves, int Size)
{
    char *Str = emalloc(sizeof(char)*RULES_MOVES_STR_LEN), *pBegin = Str;
    tSize Move = 1;
    bool StartedMove = false;

    for (tIndex i = 0; i < Size; ++i)
    {
        tIndex Index = pMoves[i];
        char *pId = board_index_id(Index);

        if (rules_index_player(pRules, i) AND NOT StartedMove)
        {
            char *MoveFmt = IF (Move < 10) THEN " %d. %s " ELSE "%d. %s ";
            Str += sprintf(Str, MoveFmt, Move, pId);
            Move++;
            StartedMove = true;
        }
        else
        {
            if (NOT rules_index_player(pRules, i) AND (i+1 < Size) 
                AND rules_index_player(pRules, i+1)) 
            {
                Str += sprintf(Str, "%s  ", pId); 

                StartedMove = false;

                if ((Move-1) % 3 == 0)
                {
                    Str += sprintf(Str, "\n");
                }  
            }
            else 
            {
                Str += sprintf(Str, "%s ", pId); 
            }
        }

        free(pId);
    }

    Str = pBegin;

Error:
    return Str;
}

static uint64_t rules_policy(tRules *pRules, tBoard *pBoard)
{
    uint64_t Res = 0ULL;

    if (board_finished(pBoard))
    {
        Res = -EINVAL;
        dbg_printf(DEBUG_LEVEL_ERROR, "Game is finshed\n");
        goto Error;
    }
    else
    {
        Res = pRules->MovePolicies[board_move(pBoard)];
    }

Error:
    return Res;
}

static bool rules_index_player(tRules *pRules, tIndex Index)
{
    bool Player = false;

    if (NOT board_index_valid(Index))
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Invalid index\n");
        goto Error;
    }

    Player = pRules->MovePolicies[Index] & RULES_PLAYER_MASK;

Error:
    return Player;
}

static void rules_by_type(tRules *pRules, eRulesType RulesType)
{
    switch (RulesType)
    {
        case RULES_CLASSICAL:
        {
            memcpy(&pRules->MovePolicies, &RulesClassical, sizeof(pRules->MovePolicies));
            break;
        }
        case RULES_MODERN:
        {
            memcpy(&pRules->MovePolicies, &RulesModern, sizeof(pRules->MovePolicies));
            break;
        }
        case RULES_EXPERIMENTAL:
        {
            memcpy(&pRules->MovePolicies, &RulesExperimental, sizeof(pRules->MovePolicies));
            break;
        }
        default: 
        {
            dbg_printf(DEBUG_LEVEL_ERROR, "Invalid rules type\n");
            break;
        }
    }
}
