#ifndef __RANDOM_H__
#define __RANDOM_H__

#include <stdint.h>

typedef struct Random
{
    uint64_t s[2];
} 
tRandom;

void random_init(tRandom *pRandom);
uint64_t random_next(tRandom *pRandom);

#endif
