#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <stdbool.h>

#include "random.h"
#include "types.h"

#define VECTOR_MAX_CAPACITY     TSIZE_MAX

typedef struct Vector
{
    void **ppItems;
    tSize Size;
    tSize Capacity;
}
tVector;

typedef struct VectorIterator
{
    void **pCurrent, *pEnd;
}
tVectorIterator;

int vector_init(tVector *pVector);
int vector_init_capacity(tVector *pVector, tSize Capacity);
int vector_init_items(tVector *pVector, void **ppItems, tSize Size);
int vector_init_items_capacity(tVector *pVector, void **ppItems, tSize Size, tSize Capacity);
void vector_free(tVector *pVector);
tSize vector_size(tVector *pVector);
tSize vector_capacity(tVector *pVector);
bool vector_empty(tVector *pVector);
bool vector_full(tVector *pVector);
bool vector_add(tVector *pVector, void *pItem);
bool vector_merge(tVector *pVector, tVector *pV);
void *vector_get(tVector *pVector, tIndex Index);
void *vector_set(tVector *pVector, tIndex Index, void *pItem);
void *vector_take(tVector *pVector, tIndex Index);
void *vector_pop(tVector *pVector);
void vector_shuffle(tVector *pVector, tRandom *pRand);

void vector_iterator_init(tVectorIterator *pIterator, tVector* pVector);
bool vector_iterator_has_next(tVectorIterator *pIterator);
void *vector_iterator_next(tVectorIterator *pIterator);

#endif
