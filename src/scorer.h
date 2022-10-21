#ifndef __SCORER_H__
#define __SCORER_H__

#include "board.h"
#include "types.h"

void scorer_init();
void scorer_free();

tScore score(tBoard *pBoard);

#endif