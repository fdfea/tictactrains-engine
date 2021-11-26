#include <errno.h>
#include <stdlib.h>

#include "debug.h"
#include "types.h"
#include "util.h"
#include "vector.h"

int vector_init(tVector *pVector)
{
    int Res = 0;

    pVector->ppItems = malloc(sizeof(void*) * VECTOR_MAX_SIZE);
    if (pVector->ppItems IS NULL)
    {
        Res = -ENOMEM;
        dbg_printf(DEBUG_ERROR, "No memory available\n");
        goto Error;
    }

    pVector->Size = 0;

Error:
    return Res;
}

void vector_free(tVector *pVector)
{
    if (pVector->ppItems ISNOT NULL)
    {
        free(pVector->ppItems);
        pVector->ppItems = NULL;
    }

    pVector->Size = 0;
}

tSize vector_size(tVector *pVector)
{
    return pVector->Size;
}

bool vector_empty(tVector *pVector)
{
    return pVector->Size == 0;
}

bool vector_add(tVector *pVector, void *pItem)
{
    bool Added = false;

    if (pVector->Size < VECTOR_MAX_SIZE) 
    {
        pVector->ppItems[pVector->Size++] = pItem;
        Added = true;
    }
    else
    {
        dbg_printf(DEBUG_WARN, "Vector reached max capacity\n");
    }

    return Added;
}

void *vector_get(tVector *pVector, tIndex Index)
{
    void *pTmp = NULL;

    if (Index >= 0 AND Index < pVector->Size)
    {
        pTmp = pVector->ppItems[Index];
    }
    else
    {
        dbg_printf(DEBUG_WARN, "Vector index out of bounds\n");
    }

    return pTmp;
}

void *vector_set(tVector *pVector, tIndex Index, void *pItem)
{
    void *pTmp = NULL;

    if (Index >= 0 AND Index < pVector->Size)
    {
        pTmp = pVector->ppItems[Index];
        pVector->ppItems[Index] = pItem;
    }
    else
    {
        dbg_printf(DEBUG_WARN, "Vector index out of bounds\n");
    }

    return pTmp;
}

void *vector_delete(tVector *pVector, tIndex Index)
{
    void *pTmp = NULL;

    if (Index >= 0 AND Index < pVector->Size)
    {
        pTmp = pVector->ppItems[Index];

        for (tIndex i = Index; i < pVector->Size - 1; ++i) 
        {
            pVector->ppItems[i] = pVector->ppItems[i + 1];
        }
        pVector->ppItems[--pVector->Size] = NULL;
    }
    else
    {
        dbg_printf(DEBUG_WARN, "Vector index out of bounds\n");
    }

    return pTmp;
}

void vector_shuffle(tVector *pVector, tRandom *pRand)
{
    void *pTmp;
    tIndex i, j;

    for (i = pVector->Size - 1; i > 0; --i) 
    {
        j = rand_next(pRand) % (i + 1);
        pTmp = vector_set(pVector, i, vector_get(pVector, j));
        (void) vector_set(pVector, j, pTmp);
    }
}
