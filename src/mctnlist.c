#include <stdbool.h>
#include <stdlib.h>

#include "board.h"
#include "mctn.h"
#include "mctnlist.h"
#include "random.h"
#include "types.h"
#include "util.h"

void mctnlist_init(tMctnList *pList, tBoard *pStates, tSize Size)
{
    pList->Size = 0;
}

void mctnlist_free(tMctnList *pList)
{
    for (tIndex i = 0; i < pList->Size; ++i)
    {
        mctn_free(&pList->pItems[i]);
    }

    free(pList->pItems);
}

tSize mctnlist_size(tMctnList *pList)
{
    return pList->Size;
}

bool mctnlist_empty(tMctnList *pList)
{
    return pList->Size == 0;
}

void mctnlist_expand(tMctnList *pList, tBoard *pStates, tSize Size)
{
    pList->pItems = emalloc(Size * sizeof(tMctn));
    pList->Size = Size;

    for (tIndex i = 0; i < Size; ++i)
    {
        mctn_init(&pList->pItems[i], &pStates[i]);
    }
}


tMctn *mctnlist_get(tMctnList *pList, tIndex Index)
{
    return &pList->pItems[Index];
}

tMctn mctnlist_delete(tMctnList *pList, tMctn Node)
{
    tMctn Tmp = Node;

    for (tIndex i = 0; i < pList->Size; ++i)
    {
        if (mctn_equals(&Tmp, &pList->pItems[i]))
        {
            Tmp = mctnlist_set(pList, i, pList->pItems[--pList->Size]);
            break;
        }
    }

    return Tmp;
}

tMctn mctnlist_set(tMctnList *pList, tIndex Index, tMctn Node)
{
    tMctn Tmp = pList->pItems[Index];

    pList->pItems[Index] = Node;

    return Tmp;
}

void mctnlist_shuffle(tMctnList *pList, tRandom *pRandom)
{
    tMctn Tmp;
    tIndex i, j;

    for (i = mctnlist_size(pList) - 1; i > 0; --i)
    {
        j = random_next(pRandom) % (i + 1);
        Tmp = *mctnlist_get(pList, i);
        mctnlist_set(pList, i, *mctnlist_get(pList, j));
        mctnlist_set(pList, j, Tmp);
    }
}
