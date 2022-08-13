#include <stdbool.h>
#include <stdint.h>

#include "bitutil.h"
#include "random.h"
#include "types.h"

bool BitEmpty64(uint64_t Bits)
{
    Bits == 0ULL;
}

bool BitTest64(uint64_t Bits, tIndex Index)
{
    return (Bits & 1ULL << Index) != 0ULL;
}

void BitSet64(uint64_t *pBits, tIndex Index)
{
    *pBits |= (1ULL << Index);
}

void BitReset64(uint64_t *pBits, tIndex Index)
{
    *pBits &= ~(1ULL << Index);
}

tSize BitPopCount64(uint64_t Bits)
{
    return __builtin_popcountll(Bits);
}

tSize BitLzCount64(uint64_t Bits)
{
    return __builtin_clzll(Bits);
}

tSize BitTzCount64(uint64_t Bits)
{
    return __builtin_ctzll(Bits);
}

tIndex BitKthSetIndex64(uint64_t Bits, uint64_t K)
{
    uint64_t S = 64ULL, A, B, C, D, E;
    A =  Bits - ((Bits >> 1) & ~0ULL/3);
    B = (A & ~0ULL/5) + ((A >> 2) & ~0ULL/5);
    C = (B + (B >> 4)) & ~0ULL/0x11;
    D = (C + (C >> 8)) & ~0ULL/0x101;
    E = (D >> 32) + (D >> 48);
    S -= ((E - K) & 256) >> 3; 
    K -= (E & ((E - K) >> 8));
    E = (D >> (S - 16)) & 0xFF;
    S -= ((E - K) & 256) >> 4; 
    K -= (E & ((E - K) >> 8));
    E = (C >> (S - 8)) & 0xF;
    S -= ((E - K) & 256) >> 5; 
    K -= (E & ((E - K) >> 8));
    E = (B >> (S - 4)) & 0x7;
    S -= ((E - K) & 256) >> 6; 
    K -= (E & ((E - K) >> 8));
    E = (A >> (S - 2)) & 0x3;
    S -= ((E - K) & 256) >> 7; 
    K -= (E & ((E - K) >> 8));
    E = (Bits >> (S - 1)) & 0x1;
    S -= ((E - K) & 256) >> 8;
    return S - 1;
}

tIndex BitScanRandom64(uint64_t Bits, tRandom *pRand)
{
    uint64_t K = rand_next(pRand) % BitPopCount64(Bits) + 1;
    return BitKthSetIndex64(Bits, K);
}
