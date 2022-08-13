#include <stdlib.h>

#include "mctn.h"
#include "mctnlist.h"

void mctnlist_init(tMctnList *pList, tBoard *pStates, tSize Size)
{
    pList->pItems = emalloc(sizeof(tMctn) * Size);
    pList->Size = Size;

    for (tIndex i = 0; i < Size; ++i)
    {
        mctn_init(&pList->pItems[i], &pStates[i]);
    }
}

void mctnlist_free(tMctnList *pList)
{
    for (tIndex i = 0; i < mctnlist_size(pList); ++i)
    {
        mctn_free(mctnlist_get(pList, i));
    }

    free(pList->pItems);
}

tSize mctnlist_size(tMctnList *pList)
{
    return pList->Size;
}

bool mctnlist_empty(tMctnList *pList)
{
    return mctnlist_size(pList) == 0;
}

tMctn *mctnlist_get(tMctnList *pList, tIndex Index)
{
    return &pList->pItems[Index];
}

void mctnlist_set(tMctnList *pList, tMctn *pNode, tIndex Index)
{
    pList->pItems[Index] = *pNode;
}

void mctnlist_delete(tMctnList *pList, tMctn *pNode)
{
    for (tIndex i = 0; i < mctnlist_size(pList); ++i)
    {
        if (mctn_equals(pNode, mctnlist_get(pList, i)))
        {
            mctnlist_set(pList, i, &pList->pItems[--pList->Size]);
            break;
        }
    }
}

void mctnlist_shuffle(tMctnList *pList, tRandom *pRand)
{
    tMctn Tmp;
    tIndex i, j;

    for (i = mctnlist_size(pList) - 1; i > 0; --i) 
    {
        j = rand_next(pRand) % (i + 1);
        Tmp = *mctnlist_get(pList, i);
        mctnlist_set(pList, i, mctnlist_get(pList, j));
        mctnlist_set(pList, j, &Tmp);
    }
}
