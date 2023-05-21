#ifndef __SCORER_H__
#define __SCORER_H__

#include "board.h"
#include "types.h"

void scorer_init();
void scorer_free();

tSize scorer_longest_path(uint64_t Data, tIndex Index, uint64_t *pArea);

#endif
