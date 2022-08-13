#ifndef __MCTNLIST_H__
#define __MCTNLIST_H__

#include <stdbool.h>

#include "board.h"
#include "random.h"
#include "types.h"

typedef struct Mctn tMctn;

typedef struct MctnList
#ifdef PACKED
__attribute__((packed))
#endif
{
    tMctn *pItems;
    tSize Size;
}
tMctnList;

void mctnlist_init(tMctnList *pList, tBoard *pStates, tSize Size);
void mctnlist_free(tMctnList *pList);
tSize mctnlist_size(tMctnList *pList);
bool mctnlist_empty(tMctnList *pList);
tMctn *mctnlist_get(tMctnList *pList, tIndex Index);
void mctnlist_set(tMctnList *pList, tIndex Index, tMctn *pNode);
void mctnlist_delete(tMctnList *pList, tMctn *pNode);
void mctnlist_shuffle(tMctnList *pList, tRandom *pRand);

#endif
