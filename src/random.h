#ifndef __RANDOM_H__
#define __RANDOM_H__

#include <stdint.h>

typedef struct Random
{
    uint64_t s[2];
} 
tRandom;

void rand_init(tRandom *pRand);
uint64_t rand_next(tRandom *pRand);

#endif
