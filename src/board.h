#ifndef __BOARD_H__
#define __BOARD_H__

#include <stdbool.h>
#include <stdint.h>

#include "bitutil.h"
#include "types.h"

#define ROWS    7
#define COLUMNS 7

#define BOARD_ID_STR_LEN    3
#define BOARD_STR_LEN       190

typedef struct Board
{
    uint64_t Data;
    uint64_t Valid;
}
tBoard;

typedef enum ScoringAlgorithm
{
    SCORING_ALGORITHM_OPTIMAL   = 1,
    SCORING_ALGORITHM_QUICK     = 2,
}
eScoringAlgorithm;

void board_init(tBoard *pBoard);
void board_copy(tBoard *pBoard, tBoard *pB);
bool board_equals(tBoard *pBoard, tBoard *pB);
void board_make_move(tBoard *pBoard, tIndex Index, bool Player);
bool board_finished(tBoard *pBoard);
tSize board_move(tBoard *pBoard);
tIndex board_last_move_index(tBoard *pBoard);
uint64_t board_valid_indices(tBoard *pBoard, uint64_t Policy, bool OnlyAdjacent);
tScore board_score(tBoard *pBoard, eScoringAlgorithm Algorithm);
char *board_string(tBoard *pBoard);
char board_index_char(tBoard *pBoard, tIndex Index);
tIndex board_id_index(char (*pId)[BOARD_ID_STR_LEN]);
char *board_index_id(tIndex Index);
bool board_id_valid(char (*pId)[BOARD_ID_STR_LEN]);
bool board_index_valid(tIndex Index);

#endif
