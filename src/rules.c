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

static const uint64_t RULES_PLAYER_MASK = 0x8000000000000000ULL;

static void rules_by_type(tRules *pRules, eRulesType RulesType);
static void rules_random_move(tRules *pRules, tBoard *pBoard, tRandom* pRand);
static bool rules_index_player(tRules *pRules, tIndex Index);

void rules_init(tRules *pRules, tRulesConfig *pConfig)
{
    rules_by_type(pRules, pConfig->RulesType);
}

void rules_config_init(tRulesConfig *pConfig)
{
    pConfig->RulesType = RULES_CLASSICAL;
}

uint64_t rules_policy(tRules *pRules, tBoard *pBoard)
{
    uint64_t Res = 0ULL;

    if (board_finished(pBoard))
    {
        Res = -EINVAL;
        dbg_printf(DEBUG_LEVEL_ERROR, "Game is finshed\n");
        goto Error;
    }

    Res = pRules->MovePolicies[board_move(pBoard)];

Error:
    return Res;
}

bool rules_player(tRules *pRules, tBoard *pBoard)
{
    return (rules_policy(pRules, pBoard) & RULES_PLAYER_MASK) != 0ULL;
}

bool rules_prev_player(tRules *pRules, tBoard *pBoard)
{
    tSize Move = board_move(pBoard);
    eMovePolicy Policy = IF (Move > 0) THEN pRules->MovePolicies[Move-1] ELSE 0ULL;

    return (Policy & RULES_PLAYER_MASK) != 0ULL;
}

void rules_next_states(tRules *pRules, tBoard *pBoard, tVector *pVector)
{
    uint64_t Policy = rules_policy(pRules, pBoard);
    uint64_t Indices = board_valid_indices(pBoard, Policy, true);
    bool Player = rules_player(pRules, pBoard);

    while (Indices != 0ULL) 
    {
        uint64_t Tmp = Indices & -Indices;
        tIndex Index = BitLzCount64(Tmp);
        tBoard *pBoardCopy = malloc(sizeof(tBoard));

        board_copy(pBoardCopy, pBoard);
        board_make_move(pBoardCopy, Index, Player);
        vector_add(pVector, pBoardCopy);

        Indices ^= Tmp;
    }
}

void rules_simulate_playout(tRules *pRules, tBoard *pBoard, tRandom *pRand)
{
    while (NOT board_finished(pBoard)) 
    {
        rules_random_move(pRules, pBoard, pRand);
    }
}

char *rules_moves_string(tRules *pRules, int *pMoves, int Size)
{
    char *Str = malloc(sizeof(char)*RULES_MOVES_STR_LEN);
    if (Str IS NULL)
    {
        dbg_printf(DEBUG_LEVEL_ERROR, "No memory available\n");
        goto Error;
    }

    char *pBegin = Str;

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

static void rules_random_move(tRules *pRules, tBoard *pBoard, tRandom* pRand)
{
    uint64_t Policy = rules_policy(pRules, pBoard);
    uint64_t ValidIndices = board_valid_indices(pBoard, Policy, true);
    tIndex RandIndex = BitScanRandom64(ValidIndices, pRand);

    board_make_move(pBoard, RandIndex, rules_player(pRules, pBoard));
}

static bool rules_index_player(tRules *pRules, tIndex Index)
{
    bool Res = false;

    if (NOT board_index_valid(Index))
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Invalid index\n");
        goto Error;
    }

    Res = pRules->MovePolicies[Index] & RULES_PLAYER_MASK;

Error:
    return Res;
}

static void rules_by_type(tRules *pRules, eRulesType RulesType)
{
    eMovePolicy MovePolicies[ROWS*COLUMNS];
    size_t PoliciesSize = ROWS*COLUMNS*sizeof(eMovePolicy);

    switch (RulesType)
    {
        case RULES_CLASSICAL:
        {
            memcpy(&MovePolicies, &RulesClassical, PoliciesSize);
            break;
        }
        case RULES_MODERN:
        {
            memcpy(&MovePolicies, &RulesModern, PoliciesSize);
            break;
        }
        case RULES_EXPERIMENTAL:
        {
            memcpy(&MovePolicies, &RulesExperimental, PoliciesSize);
            break;
        }
        default: 
        {
            dbg_printf(DEBUG_LEVEL_WARN, "Invalid rules type\n");
            memcpy(&MovePolicies, &RulesClassical, PoliciesSize);
            break;
        }
    }

    memcpy(&pRules->MovePolicies, &MovePolicies, PoliciesSize);
}
