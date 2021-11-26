#ifndef __MCTNLIST_H__
#define __MCTNLIST_H__

#include <stdbool.h>
#include <stdint.h>

#include "random.h"
#include "types.h"
#include "vector.h"

#define MCTN_LIST_MAX_SIZE  UINT8_MAX

typedef struct Mctn tMctn;

typedef struct
#ifdef PACKED
__attribute__((packed))
#endif
MctnList
{
    tMctn *pItems;
    uint8_t Size;

} tMctnList;

void mctn_list_init(tMctnList *pList);
void mctn_list_free(tMctnList *pList);
tSize mctn_list_size(tMctnList *pList);
bool mctn_list_empty(tMctnList *pList);
int mctn_list_expand(tMctnList *pList, tVector *pNextStates);
int mctn_list_add(tMctnList *pList, tMctn *pNode);
tMctn *mctn_list_get(tMctnList *pList, tIndex Index);
void mctn_list_set(tMctnList *pList, tIndex Index, tMctn *pNode);
int mctn_list_delete(tMctnList *pList, tIndex Index);
void mctn_list_shuffle(tMctnList *pList, tRandom* pRand);

#endif
