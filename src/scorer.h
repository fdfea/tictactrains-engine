#ifndef __SCORER_H__
#define __SCORER_H__

#include "board.h"
#include "types.h"

//remove
#include "vector.h"

void scorer_init();
void scorer_free();

tScore scorer_score(tBoard *pBoard);

tSize area_3x4_index_longest_path(uint16_t Data, tIndex Index);
bool area_3x4_longest_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath);
bool area_3x4_has_through_path(uint16_t Data, uint16_t PrimaryPath, tSize MaxLength);
bool area_3x4_has_path(uint16_t Data, tIndex Start, tIndex End);
void area_3x4_get_lookup_paths(uint16_t Data, tIndex Start, tIndex End, uint16_t Path, tVector *pVector, tSize MaxLength);
void area_3x4_area(uint16_t Data, tIndex Index, uint16_t *pArea);

#endif
