#ifndef __RULES_H__
#define __RULES_H__

#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "defs.h"
#include "vector.h"

#define RULES_MOVES_STR_LEN     512

typedef enum RulesType
{
    RULES_CLASSICAL     = 1,
    RULES_MODERN        = 2,
    RULES_EXPERIMENTAL  = 3,
} 
eRulesType;

typedef enum __attribute__ ((aligned(8))) MovePolicy
{
    X_ALL       = 0x8001FFFFFFFFFFFFULL, 
    X_RING1     = 0x8001FFFFFEFFFFFFULL, 
    X_RING1_U   = 0x80000001C2870000ULL, 
    X_RING1_I   = 0x80000001C3870000ULL, 
    X_RING2     = 0x8001FFFE3C78FFFFULL, 
    X_RING2_U   = 0x800001F224489F00ULL, 
    X_RING2_I   = 0x800001F3E7CF9F00ULL, 
    X_RING2_C   = 0x8001FFFE3D78FFFFULL,
    X_RING3     = 0x8001FE0C183060FFULL,  
    X_RING3_C   = 0x8001FE0C193060FFULL,
    O_ALL       = 0x0001FFFFFFFFFFFFULL, 
    O_RING1     = 0x0001FFFFFEFFFFFFULL, 
    O_RING1_U   = 0x00000001C2870000ULL, 
    O_RING1_I   = 0x00000001C3870000ULL,
    O_RING2     = 0x0001FFFE3C78FFFFULL, 
    O_RING2_U   = 0x000001F224489F00ULL, 
    O_RING2_I   = 0x000001F3E7CF9F00ULL, 
    O_RING2_C   = 0x0001FFFE3D78FFFFULL, 
    O_RING3     = 0x0001FE0C183060FFULL,
    O_RING3_C   = 0x0001FE0C193060FFULL,  
}
eMovePolicy;
//__attribute__ ((aligned(8))) eMovePolicy;

typedef struct Rules
{
    eMovePolicy MovePolicies[ROWS*COLUMNS];
}
tRules;

typedef struct RulesConfig
{
    eRulesType RulesType;
}
tRulesConfig;

void rules_init(tRules *pRules, tRulesConfig *pConfig);
void rules_config_init(tRulesConfig *pConfig);
uint64_t rules_policy(tRules *pRules, tBoard *pBoard);
bool rules_player(tRules *pRules, tBoard *pBoard);
bool rules_prev_player(tRules *pRules, tBoard *pState);
void rules_simulate_playout(tRules *pRules, tBoard *pBoard, tRandom *pRand);
void rules_next_states(tRules *pRules, tBoard *pBoard, tVector *pVector);
char *rules_moves_string(tRules *pRules, int *pMoves, int Size);

static const eMovePolicy RulesClassical[ROWS*COLUMNS] = 
{
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
};

static const eMovePolicy RulesModern[ROWS*COLUMNS] = 
{
    X_ALL, O_RING1_I, O_RING3, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
};

static const eMovePolicy RulesExperimental[ROWS*COLUMNS] = 
{
    X_ALL, O_RING2, O_RING3, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
};

#endif
