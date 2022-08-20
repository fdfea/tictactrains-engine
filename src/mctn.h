#ifndef __MCTN_H__
#define __MCTN_H__

#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "mctnlist.h"
#include "random.h"

#define MCTN_STR_LEN    2048

#ifdef VISITS32
#define TVISITS_MAX     1e6
typedef uint32_t tVisits;
#else
#define TVISITS_MAX     UINT16_MAX
typedef uint16_t tVisits;
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
void mctn_copy(tMctn *pNode, tMctn *pN);
void mctn_update(tMctn *pNode, float Score);
void mctn_expand(tMctn *pNode, tBoard *pStates, tSize Size);
bool mctn_equals(tMctn *pNode, tMctn *pN);
tMctn *mctn_random_child(tMctn *pNode, tRandom *pRandom);
tMctn *mctn_most_visited_child(tMctn *pNode);
tMctn *mctn_best_child_uct(tMctn *pNode);
char *mctn_string(tMctn *pNode);

#endif
