#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "types.h"
#include "util.h"
#include "vector.h"

#define VECTOR_SCALING_FACTOR   1

static void vector_resize(tVector *pVector, tSize Size);

int vector_init(tVector *pVector, tSize Size)
{
    vector_init_capacity(pVector, VECTOR_MAX_CAPACITY);
}

int vector_init_capacity(tVector *pVector, tSize Capacity)
{
    int Res = 0;

    if (Capacity > VECTOR_MAX_CAPACITY)
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Requested size for vector exceeds max capacity\n");
        Res = -EINVAL;
        goto Error;
    }
    
    pVector->ppItems = emalloc(sizeof(void*) * Capacity);
    pVector->Size = 0;
    pVector->Capacity = Capacity;

Error:
    return Res;
}

void vector_free(tVector *pVector)
{
    free(pVector->ppItems);
    pVector->Size = 0;
    pVector->Capacity = 0;
}

tSize vector_size(tVector *pVector)
{
    return pVector->Size;
}

tSize vector_capacity(tVector *pVector)
{
    return pVector->Capacity;
}

bool vector_empty(tVector *pVector)
{
    return vector_size(pVector) == 0;
}

bool vector_full(tVector *pVector)
{
    return vector_size(pVector) == vector_capacity(pVector);
}

bool vector_add(tVector *pVector, void *pItem)
{
    bool Added = false;
    tSize Size = vector_size(pVector) + 1;

    if (Size > VECTOR_MAX_CAPACITY)
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Vector cannot exceed max capacity\n");
        goto Error;
    }

    vector_resize(pVector, Size);

    pVector->ppItems[pVector->Size++] = pItem;

Error:
    return Added;
}

bool vector_merge(tVector *pVector, tVector *pV)
{
    bool Added = false;
    tSize Size1 = vector_size(pVector), Size2 = vector_size(pV), Size = Size1 + Size2;

    if (Size > VECTOR_MAX_CAPACITY)
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Vector cannot exceed max capacity\n");
        goto Error;
    }

    vector_resize(pVector, Size);

    memcpy(&pVector->ppItems[Size1], pV->ppItems, sizeof(void*) * Size2);
    pVector->Size = Size;

Error:
    return Added;
}

void *vector_get(tVector *pVector, tIndex Index)
{
    void *pItem = NULL;

    if (Index >= 0 AND Index < vector_size(pVector))
    {
        pItem = pVector->ppItems[Index];
    }
    else
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Vector index out of bounds\n");
    }

    return pItem;
}

void *vector_take(tVector *pVector, tIndex Index)
{
    int Res = 0;
    void *pItem = NULL;

    if (Index >= 0 AND Index < vector_size(pVector))
    {
        pItem = pVector->ppItems[Index];
        pVector->ppItems[Index] = pVector->ppItems[--pVector->Size];
    }
    else
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Vector index out of bounds\n");
        goto Error;
    }

    vector_resize(pVector, vector_size(pVector));

Error:
    return pItem;
}

void *vector_get_random(tVector *pVector, tRandom *pRand)
{
    return vector_get(pVector, rand_next(pRand) % vector_size(pVector));
}

void *vector_take_random(tVector *pVector, tRandom *pRand)
{
    return vector_take(pVector, rand_next(pRand) % vector_size(pVector));
}

void *vector_set(tVector *pVector, tIndex Index, void *pItem)
{
    void *pItem = NULL;

    if (Index >= 0 AND Index < vector_size(pVector))
    {
        pItem = pVector->ppItems[Index];
        pVector->ppItems[Index] = pItem;
    }
    else
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Vector index out of bounds\n");
    }

    return pItem;
}

void vector_shuffle(tVector *pVector, tRandom *pRand)
{
    void *pTmp;
    tIndex i, j;

    for (i = vector_size(pVector) - 1; i > 0; --i) 
    {
        j = rand_next(pRand) % (i + 1);
        pTmp = vector_set(pVector, i, vector_get(pVector, j));
        (void) vector_set(pVector, j, pTmp);
    }
}

void vector_map(tVector *pVector, tMapFunction *pFunction)
{
    for (tIndex i = 0; i < vector_size(pVector); ++i)
    {
        pVector->ppItems[i] = (*pFunction)(pVector->ppItems[i]);
    }
}

void vector_foreach(tVector *pVector, tEffectFunction *pFunction)
{
    for (tIndex i = 0; i < vector_size(pVector); ++i)
    {
       (*pFunction)(pVector->ppItems[i]);
    }
}

static void vector_resize(tVector *pVector, tSize Size)
{
    tSize Resize = vector_size(pVector), Capacity = vector_capacity(pVector);

    if (Size > Capacity)
    {
        Resize = min(Size << VECTOR_SCALING_FACTOR, VECTOR_MAX_CAPACITY);
    }
    else if (Size <= Capacity >> VECTOR_SCALING_FACTOR)
    {
        Resize = Size;
    }

    if (Resize != vector_size(pVector))
    {
        pVector->ppItems = erealloc(pVector->ppItems, sizeof(void*) * Resize);
        pVector->Capacity = Resize;
    }
}
