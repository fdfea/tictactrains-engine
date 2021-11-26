#include <stdbool.h>
#include <stdint.h>

#include "bitutil.h"
#include "random.h"
#include "types.h"

bool BitTest64(uint64_t Mask, tIndex Index)
{
    return (Mask & 1ULL << Index) != 0ULL;
}

void BitSet64(uint64_t *pMask, tIndex Index)
{
    *pMask |= (1ULL << Index);
}

void BitReset64(uint64_t *pMask, tIndex Index)
{
    *pMask &= ~(1ULL << Index);
}

tSize BitPopCount64(uint64_t Mask)
{
    return (tSize) __builtin_popcountll(Mask);
}

tIndex BitLzCount64(uint64_t Mask)
{
    return (tIndex) __builtin_ctzll(Mask);
}

tIndex BitKthSetIndex64(uint64_t Mask, uint64_t Rank)
{
    uint64_t S = 64ULL, A, B, C, D, E;
    A =  Mask - ((Mask >> 1) & ~0ULL/3);
    B = (A & ~0ULL/5) + ((A >> 2) & ~0ULL/5);
    C = (B + (B >> 4)) & ~0ULL/0x11;
    D = (C + (C >> 8)) & ~0ULL/0x101;
    E = (D >> 32) + (D >> 48);
    S -= ((E - Rank) & 256) >> 3; 
    Rank -= (E & ((E - Rank) >> 8));
    E = (D >> (S - 16)) & 0xFF;
    S -= ((E - Rank) & 256) >> 4; 
    Rank -= (E & ((E - Rank) >> 8));
    E = (C >> (S - 8)) & 0xF;
    S -= ((E - Rank) & 256) >> 5; 
    Rank -= (E & ((E - Rank) >> 8));
    E = (B >> (S - 4)) & 0x7;
    S -= ((E - Rank) & 256) >> 6; 
    Rank -= (E & ((E - Rank) >> 8));
    E = (A >> (S - 2)) & 0x3;
    S -= ((E - Rank) & 256) >> 7; 
    Rank -= (E & ((E - Rank) >> 8));
    E = (Mask >> (S - 1)) & 0x1;
    S -= ((E - Rank) & 256) >> 8;
    return (tIndex) (S - 1);
}

tIndex BitScanRandom64(uint64_t Mask, tRandom *pRand)
{
    uint64_t Rank = (rand_next(pRand) % BitPopCount64(Mask))+1;
    return BitKthSetIndex64(Mask, Rank);
}
