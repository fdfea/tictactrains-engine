#ifndef __BOARD_PRED_H__
#define __BOARD_PRED_H__

#include "board.h"

typedef enum PredictionStrategy 
{
    PREDICTION_STRATEGY_NBR_LINREG = 1, 
    PREDICTION_STRATEGY_NBR_LOGREG = 2, 
    PREDICTION_STRATEGY_SHP_MLP    = 3, 
} 
ePredictionStrategy;

float board_predict_score(tBoard *pBoard, ePredictionStrategy Strategy);

#endif
