#ifndef __BOARD_H__
#define __BOARD_H__

#include <stdbool.h>
#include <stdint.h>

#include "types.h"

#define ROWS    7
#define COLUMNS 7

#define BOARD_ID_STR_LEN    3
#define BOARD_STR_LEN       190

#define BOARD_MASK  0x0001FFFFFFFFFFFFULL

typedef struct
#ifdef PACKED
__attribute__((packed))
#endif
Board
{
    uint64_t Data;
    uint64_t Empty;
    uint64_t Neighbors;
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
int board_advance(tBoard *pBoard, tIndex Index, bool Player);
bool board_finished(tBoard *pBoard);
tSize board_move(tBoard *pBoard);
uint64_t board_empty_indices(tBoard *pBoard, uint64_t Constraint, bool OnlyNeighbors);
tIndex board_last_move_index(tBoard *pBoard);
tScore board_score(tBoard *pBoard, eScoringAlgorithm Algorithm);
char *board_string(tBoard *pBoard);
char board_index_char(tBoard *pBoard, tIndex Index);
tIndex board_id_index(char (*pId)[BOARD_ID_STR_LEN]);
char *board_index_id(tIndex Index);
bool board_id_valid(char (*pId)[BOARD_ID_STR_LEN]);
bool board_index_valid(tIndex Index);
bool board_index_empty(tBoard *pBoard, tIndex Index);
bool board_index_player(tBoard *pBoard, tIndex Index);

#endif
