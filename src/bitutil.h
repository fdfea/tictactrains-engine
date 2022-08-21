#ifndef __BITUTIL_H__
#define __BITUTIL_H__

#include <stdbool.h>
#include <stdint.h>

#include "random.h"
#include "types.h"

bool BitEmpty64(uint64_t Bits);
bool BitTest64(uint64_t Bits, tIndex Index);
void BitSet64(uint64_t *pBits, tIndex Index);
void BitReset64(uint64_t *pBits, tIndex Index);
tSize BitPopCount64(uint64_t Bits);
tSize BitLzCount64(uint64_t Bits);
tSize BitTzCount64(uint64_t Bits);
tIndex BitKthSetIndex64(uint64_t Bits, uint64_t K);
tIndex BitScanRandom64(uint64_t Bits, tRandom *pRandom);

#endif
