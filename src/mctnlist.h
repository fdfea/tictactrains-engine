#ifndef __MCTNLIST_H__
#define __MCTNLIST_H__

#include <stdbool.h>

#include "board.h"
#include "random.h"
#include "types.h"

typedef struct Mctn tMctn;

typedef struct
#ifdef PACKED
__attribute__((packed))
#endif
MctnList
{
    tMctn *pItems;
    tSize Size;
}
tMctnList;

void mctnlist_init(tMctnList *pList);
void mctnlist_free(tMctnList *pList);
tSize mctnlist_size(tMctnList *pList);
bool mctnlist_empty(tMctnList *pList);
void mctnlist_expand(tMctnList *pList, tBoard *pStates, tSize Size);
tMctn *mctnlist_get(tMctnList *pList, tIndex Index);
tMctn mctnlist_delete(tMctnList *pList, tMctn Node);
tMctn mctnlist_set(tMctnList *pList, tIndex Index, tMctn Node);
void mctnlist_shuffle(tMctnList *pList, tRandom *pRandom);

#endif
