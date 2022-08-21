#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "debug.h"
#include "random.h"

void random_init(tRandom *pRandom)
{
#if defined (DEBUG_DEV) || defined (DEBUG)
    srand(DEBUG_RANDOM_SEED);
#else
    srand(time(NULL));
#endif
    pRandom->s[0] = (uint64_t) rand();
    pRandom->s[1] = (uint64_t) rand() + 1;
}

uint64_t random_next(tRandom *pRandom)
{
    uint64_t s1 = pRandom->s[0];
    uint64_t s0 = pRandom->s[1];
    pRandom->s[0] = s0;
    s1 ^= s1 << 23;
    pRandom->s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
    return pRandom->s[1] + s0;
}
