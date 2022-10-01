#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "random.h"
#include "types.h"
#include "util.h"
#include "vector.h"

#define VECTOR_SCALING_FACTOR   1
#define VECTOR_HALF_CAPACITY    (VECTOR_MAX_CAPACITY >> 1)

static void vector_resize(tVector *pVector, tSize Size, bool Grow);

int vector_init(tVector *pVector)
{
    return vector_init_capacity(pVector, 0);
}

int vector_init_capacity(tVector *pVector, tSize Capacity)
{
    return vector_init_items_capacity(pVector, NULL, 0, Capacity);
}

int vector_init_items(tVector *pVector, void **ppItems, tSize Size)
{
    return vector_init_items_capacity(pVector, ppItems, Size, Size);
}

int vector_init_items_capacity(tVector *pVector, void **ppItems, tSize Size, tSize Capacity)
{
    int Res = 0;
    
    if (Size > Capacity)
    {
        Res = -EINVAL;
        dbg_printf(DEBUG_LEVEL_WARN, "Cannot initialize vector with size greater than capacity");
        goto Error;
    }
    else if (Capacity > VECTOR_MAX_CAPACITY)
    {
        Res = -EINVAL;
        dbg_printf(DEBUG_LEVEL_WARN, "Cannot initialize vector capacity greater than max capacity");
        goto Error;
    }

    pVector->ppItems = emalloc(Capacity * sizeof(void *));

    memcpy(pVector->ppItems, ppItems, Size * sizeof(void *));

    pVector->Size = Size;
    pVector->Capacity = Capacity;

Error:
    return Res;
}

void vector_free(tVector *pVector)
{
    free(pVector->ppItems);
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
    return vector_size(pVector) >= vector_capacity(pVector);
}

bool vector_add(tVector *pVector, void *pItem)
{
    bool Added = false;
    tSize Size = vector_size(pVector), Resize = Size + 1;

    if (Size >= VECTOR_MAX_CAPACITY)
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Cannot add item to vector at max capacity");
        goto Error;
    }

    vector_resize(pVector, Resize, true);

    pVector->ppItems[Size] = pItem;
    pVector->Size = Resize;
    Added = true;

Error:
    return Added;
}

bool vector_merge(tVector *pVector, tVector *pV)
{
    bool Merged = false;
    tSize Size1 = vector_size(pVector), Size2 = vector_size(pV), Resize = Size1 + Size2;

    if (Size1 >= VECTOR_MAX_CAPACITY OR Size2 >= VECTOR_MAX_CAPACITY)
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Cannot merge vectors at max capacity");
        goto Error;
    }
    else if (Resize < Size1)
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Cannot merge vectors to exceed max capacity");
        goto Error;
    }

    vector_resize(pVector, Resize, true);

    memcpy(&pVector->ppItems[Size1], pV->ppItems, Size2 * sizeof(void *));

    pVector->Size = Resize;
    Merged = true;

Error:
    return Merged;
}

void *vector_get(tVector *pVector, tIndex Index)
{
    void *pItem = NULL;

    if (Index >= vector_size(pVector))
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Cannot get item from vector at index out of bounds");
        goto Error;
    }

    pItem = pVector->ppItems[Index];

Error:
    return pItem;
}

void *vector_set(tVector *pVector, tIndex Index, void *pItem)
{
    void *pTmp = NULL;

    if (Index >= vector_size(pVector))
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Cannot set item in vector at index out of bounds");
        goto Error;
    }

    pTmp = pVector->ppItems[Index];
    pVector->ppItems[Index] = pItem;

Error:
    return pTmp;
}

void *vector_take(tVector *pVector, tIndex Index)
{
    void *pItem = NULL;
    tSize Size = vector_size(pVector), Resize = Size - 1;

    if (Index >= Size)
    {
        dbg_printf(DEBUG_LEVEL_WARN, "Cannot take item from vector at index out of bounds");
        goto Error;
    }

    pItem = vector_set(pVector, Index, pVector->ppItems[Resize]);
    
    vector_resize(pVector, Resize, false);

    pVector->Size = Resize;

Error:
    return pItem;
}

void *vector_pop(tVector *pVector)
{
    void *pItem = NULL;

    if (NOT vector_empty(pVector))
    {
        pItem = vector_take(pVector, vector_size(pVector) - 1);
    }

    return pItem;
}

void vector_shuffle(tVector *pVector, tRandom *pRandom)
{
    void *pTmp;
    tIndex i, j;

    for (i = vector_size(pVector) - 1; i > 0; --i) 
    {
        j = random_next(pRandom) % (i + 1);
        pTmp = vector_set(pVector, j, vector_get(pVector, i));
        (void) vector_set(pVector, i, pTmp);
    }
}

static void vector_resize(tVector *pVector, tSize Size, bool Grow)
{
    tSize Resize, Capacity;
    
    Resize = Capacity = vector_capacity(pVector);

    if (Grow AND Size > Capacity)
    {
        Resize = IF (Size >= VECTOR_HALF_CAPACITY) THEN VECTOR_MAX_CAPACITY ELSE Size << VECTOR_SCALING_FACTOR;
    }
    else if (NOT Grow AND Size <= Capacity >> VECTOR_SCALING_FACTOR)
    {
        Resize = Size;
    }

    if (Resize != Capacity)
    {
        pVector->ppItems = erealloc(pVector->ppItems, Resize * sizeof(void *));
        pVector->Capacity = Resize;
    }
}
