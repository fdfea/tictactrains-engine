#ifndef __MCTN_H__
#define __MCTN_H__

#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "mctn_list.h"
#include "random.h"
#include "vector.h"

#define MCTN_STR_LEN    2048

#ifdef VISITS32
typedef uint32_t tVisits;
#define TVISITS_MAX     1e6
#else
typedef uint16_t tVisits;
#define TVISITS_MAX     UINT16_MAX
#endif

typedef struct 
#ifdef PACKED
__attribute__((packed))
#endif
Mctn
{
    tBoard State;
    tMctnList Children;
    tVisits Visits;
    float Score;
} 
tMctn;

void mctn_init(tMctn *pNode, tBoard *pBoard);
void mctn_free(tMctn *pNode);
void mctn_update(tMctn *pNode, float Score);
int mctn_expand(tMctn *pNode, tVector *pNextStates);
bool mctn_equals(tMctn *pNode, tMctn *pN);
tMctn *mctn_random_child(tMctn *pNode, tRandom *pRand);
tMctn *mctn_best_child_uct(tMctn *pNode);
tMctn *mctn_most_visited_child(tMctn *pNode);
char *mctn_string(tMctn *pNode);

#endif
