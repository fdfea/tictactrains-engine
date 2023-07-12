#ifndef __RULES_H__
#define __RULES_H__

#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "types.h"
#include "vector.h"

#define RULES_MOVES_STR_LEN     512

typedef enum RulesType
{
    RULES_CLASSICAL     = 1,
    RULES_MODERN        = 2,
    RULES_POSTMODERN    = 3,
    RULES_SPANISH       = 4,
    RULES_SWISS         = 5,
    RULES_VIENNESE      = 6,
    RULES_MALLORCAN     = 7,
} 
eRulesType;

typedef struct RulesConfig
{
    eRulesType RulesType;
}
tRulesConfig;

typedef enum __attribute__((aligned(8))) MovePolicy
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

typedef struct Rules
{
    eMovePolicy MovePolicies[ROWS*COLUMNS];
}
tRules;

void rules_init(tRules *pRules, tRulesConfig *pConfig);
void rules_config_init(tRulesConfig *pConfig);
uint64_t rules_indices(tRules *pRules, tBoard *pBoard, bool OnlyAdjacent);
bool rules_player(tRules *pRules, tBoard *pBoard);
bool rules_prev_player(tRules *pRules, tBoard *pState);
void rules_simulate_playout(tRules *pRules, tBoard *pBoard, tRandom *pRandom, bool OnlyNeighbors);
tBoard *rules_next_states(tRules *pRules, tBoard *pBoard, tSize *pSize, bool OnlyNeighbors);
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

static const eMovePolicy RulesPostmodern[ROWS*COLUMNS] = 
{
    X_ALL, O_RING1_I, O_RING2_U, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
};

static const eMovePolicy RulesSpanish[ROWS*COLUMNS] = 
{
    X_ALL, O_RING1_U, O_RING1_U, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
};

static const eMovePolicy RulesSwiss[ROWS*COLUMNS] = 
{
    X_ALL, O_RING1, O_RING2, O_RING3, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
};

static const eMovePolicy RulesViennese[ROWS*COLUMNS] = 
{
    X_ALL, O_RING2_U, O_RING3, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
};

static const eMovePolicy RulesMallorcan[ROWS*COLUMNS] = 
{
    X_ALL, O_RING1, O_RING3, O_RING3, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
    O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL,
    X_ALL, O_ALL, X_ALL, O_ALL, X_ALL, O_ALL, X_ALL,
};

#endif
