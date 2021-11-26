#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <stdbool.h>
#include <stdint.h>

#include "bitutil.h"
#include "random.h"
#include "types.h"

#define VECTOR_MAX_SIZE     UINT8_MAX

typedef struct Vector
{
    void **ppItems;
    tSize Size;
} 
tVector;

int vector_init(tVector *pVector);
void vector_free(tVector *pVector);
tSize vector_size(tVector *pVector);
bool vector_empty(tVector *pVector);
bool vector_add(tVector *pVector, void *pItem);
void *vector_get(tVector *pVector, tIndex Index);
void *vector_set(tVector *pVector, tIndex Index, void *pItem);
void *vector_delete(tVector *pVector, tIndex Index);
void vector_shuffle(tVector *pVector, tRandom* pRand);

#endif
