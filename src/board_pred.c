#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bitutil.h"
#include "board.h"
#include "board_pred.h"
#include "debug.h"
#include "defs.h"
#include "types.h"
#include "util.h"

#define BOARD_NEIGHBORS     4
#define BOARD_SHAPES        33

#define BOARD_PRED_DRAW_MARGIN      0.4f
#define BOARD_SHP_MLP_SCORE_SCALER  24

#define BOARD_SHP_MLP_NEURONS   20
#define BOARD_SHP_LOOKUP_SIZE   512

#define BOARD_WIN   1.0f
#define BOARD_DRAW  0.5f
#define BOARD_LOSS  0.0f

#define BOARD_WIN_BASE      0.90f
#define BOARD_WIN_BONUS     0.025f
#define BOARD_LOSS_BASE     0.10f
#define BOARD_LOSS_PENALTY  0.025f

typedef struct BoardNbrArea
{
    tSize N[BOARD_NEIGHBORS];
}
tBoardNbrArea;

typedef struct BoardShpArea
{
    tSize S[BOARD_SHAPES];
}
tBoardShpArea;

static const float NbrLinRegCoeffs[BOARD_NEIGHBORS];
static const float NbrLinRegIntercept = 3.0730f;

static const float NbrLogRegCoeffsX[BOARD_NEIGHBORS];
static const float NbrLogRegCoeffsO[BOARD_NEIGHBORS];
static const float NbrLogRegIntercept = 0.2829f;

static const uint8_t ShpLookupTable[BOARD_SHP_LOOKUP_SIZE];
static const float ShpMlpCoeffs_H[BOARD_SHP_MLP_NEURONS][BOARD_SHAPES];
static const float ShpMlpIntercepts_H[BOARD_SHP_MLP_NEURONS];
static const float ShpMlpCoeffs_O[BOARD_SHP_MLP_NEURONS];
static const float ShpMlpIntercept_O = 0.32672733f;

static void board_nbra_init(tBoardNbrArea *pArea);
static void board_nbra_inc(tBoardNbrArea *pArea, tBoard *pBoard, tIndex Index);
static void board_nbra_gen(tBoardNbrArea *pArea, tBoard *pBoard, tIndex Index, uint64_t *pChecked);

static void board_shpa_init(tBoardShpArea *pArea);
static void board_shpa_inc(tBoardShpArea *pArea, tBoard *pBoard, tIndex Index);
static void board_shpa_gen(tBoardShpArea *pArea, tBoard *pBoard, tIndex Index, uint64_t *pChecked);

static float board_predict_nbr_linreg(tBoard *pBoard);
static float board_predict_nbr_logreg(tBoard *pBoard);
static float board_predict_shp_mlp(tBoard *pBoard);

static float board_eval_nbr_linreg(tBoardNbrArea *pArea);
static float board_eval_nbr_logreg(tBoardNbrArea *pAreaX, tBoardNbrArea *pAreaO);
static float board_eval_shp_mlp(tBoardShpArea *pArea);

static float ReLU(float X);

float board_predict_score(tBoard *pBoard, ePredictionStrategy Strategy)
{
    float Res = 0.0f;

    if (NOT board_finished(pBoard))
    {
        dbg_printf(DEBUG_WARN, "Game not finished\n");
    }

    switch (Strategy) 
    {
        case PREDICTION_STRATEGY_NBR_LINREG: 
        {
            Res = board_predict_nbr_linreg(pBoard);
            break;
        }
        case PREDICTION_STRATEGY_NBR_LOGREG: 
        {
            Res = board_predict_nbr_logreg(pBoard);
            break;
        }
        case PREDICTION_STRATEGY_SHP_MLP:
        {
            Res = board_predict_shp_mlp(pBoard);
            break;
        }
        default:
        {
            dbg_printf(DEBUG_WARN, "Invalid prediction strategy\n");
            break;
        }
    }

    return Res;
}

static void board_nbra_init(tBoardNbrArea *pArea)
{
    memset(pArea->N, 0, sizeof(pArea->N));
}

static void board_nbra_inc(tBoardNbrArea *pArea, tBoard *pBoard, tIndex Index) 
{
    if (board_index_empty(pBoard, Index))
    {
        dbg_printf(DEBUG_WARN, "Index empty\n");
    }

    bool Player = board_index_player(pBoard, Index);
    tSize Rank = (board_index_left_valid(Index) 
            AND board_index_player(pBoard, board_index_left(Index)) == Player)
        + (board_index_right_valid(Index)
            AND board_index_player(pBoard, board_index_right(Index)) == Player) 
        + (board_index_top_valid(Index)
            AND board_index_player(pBoard, board_index_top(Index)) == Player)
        + (board_index_bottom_valid(Index)
            AND board_index_player(pBoard, board_index_bottom(Index)) == Player);

    pArea->N[Rank]++;
}

static void board_nbra_gen(tBoardNbrArea *pArea, tBoard *pBoard, tIndex Index, uint64_t *pChecked) 
{
    if (board_index_empty(pBoard, Index))
    {
        dbg_printf(DEBUG_WARN, "Index empty\n");
    }

    bool Player = board_index_player(pBoard, Index);
    BitSet64(pChecked, Index);
    board_nbra_inc(pArea, pBoard, Index);

    if (board_index_left_valid(Index) 
        AND NOT board_index_empty(pBoard, board_index_left(Index))
        AND board_index_player(pBoard, board_index_left(Index)) == Player 
        AND NOT BitTest64(*pChecked, board_index_left(Index)))
    {
        board_nbra_gen(pArea, pBoard, board_index_left(Index), pChecked);
    }
    if (board_index_right_valid(Index) 
        AND NOT board_index_empty(pBoard, board_index_right(Index))
        AND board_index_player(pBoard, board_index_right(Index)) == Player 
        AND NOT BitTest64(*pChecked, board_index_right(Index)))
    {
        board_nbra_gen(pArea, pBoard, board_index_right(Index), pChecked);
    }
    if (board_index_top_valid(Index) 
        AND NOT board_index_empty(pBoard, board_index_top(Index))
        AND board_index_player(pBoard, board_index_top(Index)) == Player 
        AND NOT BitTest64(*pChecked, board_index_top(Index)))
    {
        board_nbra_gen(pArea, pBoard, board_index_top(Index), pChecked);
    }
    if (board_index_bottom_valid(Index) 
        AND NOT board_index_empty(pBoard, board_index_bottom(Index))
        AND board_index_player(pBoard, board_index_bottom(Index)) == Player 
        AND NOT BitTest64(*pChecked, board_index_bottom(Index)))
    {
        board_nbra_gen(pArea, pBoard, board_index_bottom(Index), pChecked);
    }
}

static void board_shpa_init(tBoardShpArea *pArea)
{
    memset(pArea->S, 0, sizeof(pArea->S));
}

static void board_shpa_inc(tBoardShpArea *pArea, tBoard *pBoard, tIndex Index)
{
    if (board_index_empty(pBoard, Index))
    {
        dbg_printf(DEBUG_WARN, "Index empty\n");
    }

    bool Piece = board_index_player(pBoard, Index),
        LeftValid = board_index_left_valid(Index), 
        RightValid = board_index_right_valid(Index), 
        TopValid = board_index_top_valid(Index), 
        BottomValid = board_index_bottom_valid(Index), 
        Left = LeftValid AND board_index_player(pBoard, board_index_left(Index)) == Piece, 
        Right = RightValid AND board_index_player(pBoard, board_index_right(Index)) == Piece, 
        Top = TopValid AND board_index_player(pBoard, board_index_top(Index)) == Piece, 
        Bottom = BottomValid AND board_index_player(pBoard, board_index_bottom(Index)) == Piece, 
        LeftTop = (Left OR Top) AND (LeftValid AND TopValid) 
            AND board_index_player(pBoard, board_index_top(board_index_left(Index))), 
        LeftBottom = (Left OR Bottom) AND (LeftValid AND BottomValid) 
            AND board_index_player(pBoard, board_index_bottom(board_index_left(Index))), 
        RightTop = (Right OR Top) AND (RightValid AND TopValid) 
            AND board_index_player(pBoard, board_index_top(board_index_right(Index))), 
        RightBottom = (Right OR Bottom) AND (RightValid AND BottomValid) 
            AND board_index_player(pBoard, board_index_bottom(board_index_right(Index)));

    uint16_t Code = LeftTop << 8 | Top << 7 | RightTop << 6 | Left << 5 
        | 1 << 4 | Right << 3 | LeftBottom << 2 | Bottom << 1 | RightBottom;

    uint8_t Type = ShpLookupTable[Code];

    if (Type < BOARD_SHAPES)
    {
        pArea->S[Type]++;
    }
    else
    {
        dbg_printf(DEBUG_WARN, "Unknown shape type\n");
    }
}

static void board_shpa_gen(tBoardShpArea *pArea, tBoard *pBoard, tIndex Index, uint64_t *pChecked)
{
    if (board_index_empty(pBoard, Index))
    {
        dbg_printf(DEBUG_WARN, "Index empty\n");
    }

    bool Player = board_index_player(pBoard, Index);
    BitSet64(pChecked, Index);
    board_shpa_inc(pArea, pBoard, Index);

    if (board_index_left_valid(Index) 
        AND NOT board_index_empty(pBoard, board_index_left(Index))
        AND board_index_player(pBoard, board_index_left(Index)) == Player 
        AND NOT BitTest64(*pChecked, board_index_left(Index)))
    {
        board_shpa_gen(pArea, pBoard, board_index_left(Index), pChecked);
    }
    if (board_index_right_valid(Index) 
        AND NOT board_index_empty(pBoard, board_index_right(Index))
        AND board_index_player(pBoard, board_index_right(Index)) == Player 
        AND NOT BitTest64(*pChecked, board_index_right(Index)))
    {
        board_shpa_gen(pArea, pBoard, board_index_right(Index), pChecked);
    }
    if (board_index_top_valid(Index) 
        AND NOT board_index_empty(pBoard, board_index_top(Index))
        AND board_index_player(pBoard, board_index_top(Index)) == Player 
        AND NOT BitTest64(*pChecked, board_index_top(Index)))
    {
        board_shpa_gen(pArea, pBoard, board_index_top(Index), pChecked);
    }
    if (board_index_bottom_valid(Index) 
        AND NOT board_index_empty(pBoard, board_index_bottom(Index))
        AND board_index_player(pBoard, board_index_bottom(Index)) == Player 
        AND NOT BitTest64(*pChecked, board_index_bottom(Index)))
    {
        board_shpa_gen(pArea, pBoard, board_index_bottom(Index), pChecked);
    }
}

static float board_predict_nbr_linreg(tBoard *pBoard) 
{
    float Res, Score, ScoreSq, ScoreX = 0.0f, ScoreO = 0.0f;
    tBoardNbrArea Area;
    uint64_t Checked = 0ULL;

    for (tIndex i = 0; i < ROWS*COLUMNS; ++i) 
    {
        if (NOT board_index_empty(pBoard, i) AND NOT BitTest64(Checked, i)) 
        {
            board_nbra_init(&Area);
            board_nbra_gen(&Area, pBoard, i, &Checked);
            ScoreSq = board_eval_nbr_linreg(&Area);

            if (board_index_player(pBoard, i)) 
            {
                SET_IF_GREATER(ScoreSq, ScoreX);
            }
            else 
            {
                SET_IF_GREATER(ScoreSq, ScoreO);
            }
        }
    }
    Score = ScoreX - ScoreO;

    if (Score > BOARD_PRED_DRAW_MARGIN) 
    {
        float Bonus = Score * BOARD_WIN_BONUS;
        Res = BOARD_WIN_BASE + Bonus > BOARD_WIN
            ? BOARD_WIN : BOARD_WIN_BASE + Bonus;
    }
    else if (Score < -BOARD_PRED_DRAW_MARGIN)
    {
        float Penalty = Score * BOARD_LOSS_PENALTY;
        Res = BOARD_LOSS_BASE + Penalty < BOARD_LOSS
            ? BOARD_LOSS : BOARD_LOSS_BASE + Penalty;
    }
    else
    {
        Res = BOARD_DRAW;
    }  

    return Res;
}

static float board_predict_nbr_logreg(tBoard *pBoard)
{
    float Res, ScoreSq, ScoreX = 0.0f, ScoreO = 0.0f;
    tBoardNbrArea AreaX, AreaO;
    uint64_t Checked = 0ULL;
    board_nbra_init(&AreaX);
    board_nbra_init(&AreaO);

    for (tIndex i = 0; i < ROWS*COLUMNS; ++i) 
    {
        if (NOT board_index_empty(pBoard, i) AND NOT BitTest64(Checked, i)) 
        {
            tBoardNbrArea Area;
            board_nbra_init(&Area);
            board_nbra_gen(&Area, pBoard, i, &Checked);
            ScoreSq = board_eval_nbr_linreg(&Area);

            if (board_index_player(pBoard, i)) 
            {
                SET_IF_GREATER_W_EXTRA(ScoreSq, ScoreX, Area, AreaX);
            }
            else 
            {
                SET_IF_GREATER_W_EXTRA(ScoreSq, ScoreO, Area, AreaO);
            }
        }
    }

    Res = 1.0 - board_eval_nbr_logreg(&AreaX, &AreaO);
    return Res;
}

static float board_predict_shp_mlp(tBoard *pBoard)
{
    float Res, Score, ScoreSq, ScoreX = 0.0f, ScoreO = 0.0f;
    tBoardShpArea Area;
    uint64_t Checked = 0ULL;

    for (tIndex i = 0; i < ROWS*COLUMNS; ++i) 
    {
        if (NOT board_index_empty(pBoard, i) AND NOT BitTest64(Checked, i)) 
        {
            board_shpa_init(&Area);
            board_shpa_gen(&Area, pBoard, i, &Checked);
            ScoreSq = board_eval_shp_mlp(&Area);

            if (board_index_player(pBoard, i)) 
            {
                SET_IF_GREATER(ScoreSq, ScoreX);
            }
            else 
            {
                SET_IF_GREATER(ScoreSq, ScoreO);
            }
        }
    }
    Score = (ScoreX - ScoreO) * BOARD_SHP_MLP_SCORE_SCALER;

    if (Score > BOARD_PRED_DRAW_MARGIN) 
    {
        float Bonus = Score * BOARD_WIN_BONUS;
        Res = BOARD_WIN_BASE + Bonus > BOARD_WIN
            ? BOARD_WIN : BOARD_WIN_BASE + Bonus;
    }
    else if (Score < -BOARD_PRED_DRAW_MARGIN) 
    {
        float Penalty = Score * BOARD_LOSS_PENALTY;
        Res = BOARD_LOSS_BASE + Penalty < BOARD_LOSS
            ? BOARD_LOSS : BOARD_LOSS_BASE + Penalty;
    }
    else 
    {
        Res = BOARD_DRAW;
    } 

    return Res;
}

static float board_eval_nbr_linreg(tBoardNbrArea *pArea)
{
    float Prediction = NbrLinRegIntercept;
    for (tIndex i = 0; i < BOARD_NEIGHBORS; ++i)
    {
        Prediction += NbrLinRegCoeffs[i] * pArea->N[i];
    }
    return Prediction;
}

static float board_eval_nbr_logreg(tBoardNbrArea *pAreaX, tBoardNbrArea *pAreaO)
{
    float Prediction = NbrLogRegIntercept;
    for (tIndex i = 0; i < BOARD_NEIGHBORS; ++i)
    {
        Prediction += NbrLogRegCoeffsX[i] * pAreaX->N[i] + NbrLogRegCoeffsO[i] * pAreaO->N[i];
    }
    return 1 / (1 + expf(Prediction));
}

static float board_eval_shp_mlp(tBoardShpArea *pArea)
{
    tIndex i, j;
    float Hidden, Output = ShpMlpIntercept_O;

    for (i = 0; i < BOARD_SHP_MLP_NEURONS; ++i)
    {
        Hidden = ShpMlpIntercepts_H[i];
        for (j = 0; j < BOARD_SHAPES; ++j)
        {
            Hidden += pArea->S[j] * ShpMlpCoeffs_H[i][j];
        }
        Hidden = ReLU(Hidden);
        Output += Hidden * ShpMlpCoeffs_O[i];
    }

    Output = ReLU(Output);
    return Output;
}

static float ReLU(float X)
{
    return X > 0.0f ? X : 0.0f;
}

static const float NbrLinRegCoeffs[BOARD_NEIGHBORS] = {
    -0.1257f, 0.7638f, 0.9501f, 0.9388f,
};

static const float NbrLogRegCoeffsX[BOARD_NEIGHBORS] = {
    0.0750f, 1.8321f, 2.0563f, 1.8195f,
};

static const float NbrLogRegCoeffsO[BOARD_NEIGHBORS] = {
    -0.1486f, -1.9463f, -2.0657f, -1.7624f,
};

static const uint8_t ShpLookupTable[BOARD_SHP_LOOKUP_SIZE] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0xFF,0x01,0x04,0xFF,0xFF,0x04,0x06,0x01,0x04,0x03,0x07,0xFF,0xFF,0x08,0x10,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x01,0xFF,0x03,0x08,0x04,0xFF,0x07,0x10,0x02,0x05,0x09,0x0F,0x05,0x0E,0x0F,0x17,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x04,0x06,0x08,0x10,0xFF,0xFF,0x11,0x18,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x05,0x0B,0x0D,0x13,0x0C,0x16,0x14,0x1B,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x01,0xFF,0x02,0x05,0xFF,0xFF,0x05,0x0B,0x03,0x08,0x09,0x0F,0xFF,0xFF,0x0D,0x13,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x03,0xFF,0x09,0x0D,0x08,0xFF,0x0F,0x13,0x09,0x0D,0x0A,0x12,0x0D,0x15,0x12,0x1C,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x04,0xFF,0x05,0x0E,0xFF,0xFF,0x0C,0x16,0x07,0x10,0x0F,0x17,0xFF,0xFF,0x14,0x1B,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x08,0xFF,0x0D,0x15,0x11,0xFF,0x14,0x1D,0x0F,0x13,0x12,0x1C,0x14,0x1D,0x19,0x1E,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x04,0xFF,0x08,0x11,0x06,0xFF,0x10,0x18,0x05,0x0C,0x0D,0x14,0x0B,0x16,0x13,0x1B,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0E,0x16,0x15,0x1D,0x16,0x1A,0x1D,0x1F,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x04,0xFF,0x05,0x0C,0xFF,0xFF,0x0E,0x16,0x08,0x11,0x0D,0x14,0xFF,0xFF,0x15,0x1D,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x07,0xFF,0x0F,0x14,0x10,0xFF,0x17,0x1B,0x0F,0x14,0x12,0x19,0x13,0x1D,0x1C,0x1E,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x06,0xFF,0x0B,0x16,0xFF,0xFF,0x16,0x1A,0x10,0x18,0x13,0x1B,0xFF,0xFF,0x1D,0x1F,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x10,0xFF,0x13,0x1D,0x18,0xFF,0x1B,0x1F,0x17,0x1B,0x1C,0x1E,0x1B,0x1F,0x1E,0x20,
};

static const float ShpMlpIntercepts_H[BOARD_SHP_MLP_NEURONS] = {
    0.22004617f, -0.21527907f, 0.17353951f, -0.39200036f, -0.32325032f, 0.03488529f, 
    -0.33662159f, 0.17709508f, -0.12926784f, 0.03624839f, -0.07916909f, 0.39932054f, 
    -0.16327261f, -0.14874382f, 0.28474453f, 0.11027989f, 0.0917148f, -0.15712909f, 
    0.09466898f, -0.04215419f,
};

static const float ShpMlpCoeffs_O[BOARD_SHP_MLP_NEURONS] = {
    -0.19869722f, 0.38034246f, 0.1074154f, -0.35314906f, -0.29083603f, -0.09071866f, 
    0.18813615f, 0.09848193f, 0.12440304f, 0.11531291f, 0.09343664f, -0.09142089f, 
    0.13590962f, 0.40253506f, -0.19073169f, -0.35477853f, 0.19805773f, 0.08384999f, 
    -0.09968812f, -0.14651377f,
};

static const float ShpMlpCoeffs_H[BOARD_SHP_MLP_NEURONS][BOARD_SHAPES] = {
    {-2.41858030e-01f, 3.53340811e-01f, 9.66497667e-02f, -2.08078721e-01f, 
    2.95393571e-02f, -1.87395042e-01f, 3.87244459e-02f, -2.09048043e-02f, 
    6.07999229e-02f, -1.66832239e-03f, 1.34466798e-01f, -4.79263415e-01f, 
    1.68404722e-02f, 1.41585384e-02f, -1.57238781e-01f, -3.21050096e-02f, 
    -8.28341214e-02f, -8.56921537e-02f, -5.69603497e-02f, 2.78031474e-01f, 
    -1.00064090e-01f, 2.86269529e-01f, -1.90715406e-01f, -1.13846849e-01f, 
    6.27908114e-02f, 6.26218447e-02f, 3.33679606e-02f, -1.39749371e-01f, 
    1.17892307e-01f, 4.69382126e-02f, -2.65849442e-02f, -1.25974865e-01f, 
    7.40919416e-02f,}, 
    {1.19873200e-01f, 4.42031442e-02f, 5.31051769e-01f, 1.52670491e-01f, 
    1.97879280e-01f, 3.82992291e-01f, 3.50011327e-01f, 1.90552079e-01f, 
    1.32539316e-01f, -7.20086574e-02f, -1.33702924e-01f, 3.66748722e-01f, 
    5.08419945e-01f, 6.25831974e-02f, 2.44519814e-01f, 5.87679605e-01f, 
    4.88257836e-01f, 4.73319928e-01f, 7.09609975e-02f, 2.31454697e-01f, 
    2.74600351e-01f, 2.58113491e-01f, 3.86079499e-01f, 2.09373774e-01f, 
    1.44117126e-01f, 8.89865872e-02f, 2.43885556e-01f, 4.06412587e-01f, 
    3.52946774e-01f, 3.16375657e-01f, 4.54230505e-01f, 6.77228693e-02f, 
    -1.43832445e-01f,}, 
    {-2.03518588e-01f, -2.76391265e-01f, 2.27360190e-02f, -4.57442550e-01f, 
    3.68886233e-05f, 9.42129492e-02f, -4.42067568e-01f, -2.81156748e-01f, 
    -2.72912296e-01f, 1.97975563e-01f, -3.52511606e-02f, 9.70603963e-02f, 
    -3.29892250e-02f, 2.58295008e-01f, 9.38465566e-02f, -5.81724434e-02f, 
    -2.79559240e-01f, -1.35509748e-01f, -2.01327336e-01f, 1.30204746e-01f, 
    2.49910885e-01f, 1.30998506e-01f, 1.54548076e-01f, 7.18611087e-02f, 
    1.99637533e-01f, -2.84660706e-01f, 6.29030880e-01f, -3.19334318e-02f, 
    -4.02575196e-02f, 4.81194056e-02f, 1.73897150e-01f, 5.74144731e-01f, 
    5.82348757e-01f,}, 
    {-9.10845336e-03f, 4.99431640e-01f, 8.61750360e-02f, 2.10621084e-01f, 
    6.06608945e-01f, 6.68848714e-02f, 2.83009442e-01f, 4.63948038e-01f, 
    1.21602919e-01f, -1.48513699e-01f, -2.74381213e-02f, -3.10648496e-02f, 
    1.14772628e-01f, -2.33036457e-02f, 5.95181348e-02f, -7.86076949e-02f, 
    2.17781792e-01f, 1.10996068e-01f, 1.67528249e-03f, -9.44274495e-02f, 
    9.11501031e-03f, 3.08883578e-02f, -1.02491322e-02f, -8.53964362e-02f, 
    1.65619468e-01f, 7.20371929e-02f, -8.06323567e-02f, -4.59010788e-02f, 
    1.35688234e-02f, 1.29036173e-02f, -9.86845511e-02f, 6.43434078e-03f, 
    6.56861315e-04f,}, 
    {-2.19750534e-01f, 4.03831388e-01f, -6.83107690e-03f, 1.64654938e-01f, 
    6.81715781e-01f, -2.00258378e-03f, -3.70389022e-01f, 4.37338281e-01f, 
    1.65557875e-01f, -5.67529314e-01f, 1.57737067e-01f, 2.04317107e-01f, 
    -1.42675091e-02f, -4.13345303e-01f, -2.00765172e-01f, -1.56492837e-02f, 
    2.41022288e-01f, 7.49271361e-02f, 2.80401800e-01f, -2.05561034e-01f, 
    -1.20383326e-01f, 4.69019556e-01f, 2.23732876e-01f, -3.25349892e-02f, 
    -1.34264507e-02f, -5.04438009e-03f, 1.26174301e-01f, -1.45690533e-01f, 
    -1.04423304e-01f, -4.21323655e-01f, 5.64406152e-03f, -2.32168164e-01f, 
    3.46823512e-02f,}, 
    {-2.52059037e-01f, 2.30753090e-01f, 3.51768905e-02f, -8.14458440e-02f, 
    2.89562931e-01f, -3.78253144e-02f, 2.11761266e-01f, 5.37706771e-01f, 
    -1.37972628e-02f, -4.57443679e-02f, -1.13769548e-01f, 7.80714757e-03f, 
    6.03738944e-02f, -2.59652288e-02f, -6.11439939e-02f, 3.36018591e-02f, 
    5.56149480e-01f, 6.89857033e-02f, 1.21176655e-01f, -1.80142819e+00f, 
    -4.13974654e-01f, -3.53999892e-02f, -1.31421802e-01f, 6.03349254e-02f, 
    -1.45145303e-01f, -2.74568545e-01f, -1.51915307e-01f, -5.52191940e-01f, 
    1.43699420e-01f, -2.46497441e+00f, 1.98211082e-01f, -5.34429919e-01f, 
    -1.09764805e-01f,}, 
    {-1.44568753e-01f, 4.31753378e-01f, -1.35295370e-01f, 2.44225204e-01f, 
    4.15024803e-01f, -2.45222799e-01f, -3.38497427e-01f, 3.10177837e-01f, 
    2.32273603e-01f, -3.70964827e-01f, -2.05465450e-02f, 4.02983615e-01f, 
    -3.27690126e-02f, -2.33304928e-01f, -1.87553688e-01f, -9.31832499e-02f, 
    1.98092035e-01f, -7.72296785e-02f, 7.91649608e-02f, -1.43379363e-01f, 
    -2.06495761e-01f, 6.90027151e-01f, 2.54753866e-01f, -1.26665664e-01f, 
    1.32351133e-01f, -8.22205864e-02f, 2.27262284e-01f, -1.01128273e-01f, 
    -1.02664269e-01f, -3.01243959e-01f, -8.57912641e-02f, -3.45314355e-02f, 
    2.39005685e-02f,}, 
    {-2.88681206e-01f, 2.89540882e-02f, 1.23487790e-01f, -3.17339330e-02f, 
    8.06539110e-02f, 1.31947937e-01f, 4.86436954e-03f, -1.44657505e+00f, 
    5.70744993e-02f, -6.55064641e-03f, 1.25897918e-01f, -1.17654185e-01f, 
    1.01058971e-01f, -5.40701142e-02f, 8.46701692e-02f, -1.35438430e-01f, 
    2.60511075e-01f, 4.57288130e-02f, -3.56829525e-01f, -3.16961549e-02f, 
    3.12975368e-01f, -6.09485580e-02f, -1.66155739e-02f, -1.09643455e+00f, 
    -6.79052105e-01f, -3.41412178e-01f, -6.85288334e-02f, -5.10595186e-01f, 
    -5.39607307e-01f, 7.18392011e-02f, -2.59526020e-01f, -1.23719418e+00f, 
    6.46335129e-01f,}, 
   {3.85343857e-02f, 3.41268152e-01f, 9.55187570e-03f, 2.00406190e-01f, 
   4.30265511e-01f, 1.17971107e-02f, 5.11301464e-01f, 4.12868440e-01f, 
   5.03055756e-01f, 8.22685935e-02f, 1.26339191e-02f, -2.41806503e-01f, 
   3.91343349e-02f, -5.92147216e-01f, 2.55939727e-02f, -2.38994669e-02f, 
   2.78470072e-01f, 1.61911482e-02f, -7.60232505e-02f, -4.36714564e-01f, 
   -2.29536646e-01f, -4.86189503e-01f, -3.20071368e-01f, 9.15255492e-02f, 
   -4.31488856e-02f, -2.38320963e-01f, -4.46930700e-01f, -2.23726577e-01f, 
   -6.06487598e-03f, -6.91294881e-01f, 5.43939256e-02f, -1.83215622e-01f, 
   3.30674357e-02f,}, 
  {4.51341309e-02f, -1.92321399e-01f, -2.25409922e-02f, -6.19615107e-02f, 
  -1.91949493e-01f, 3.43249970e-03f, -1.96071176e-01f, -1.61825783e-01f, 
  -6.26877381e-02f, -2.38317152e-02f, -2.99423436e-02f, 1.59335517e-01f, 
  -8.16465521e-03f, 8.34843624e-03f, -1.89861695e-03f, 3.36145495e-01f, 
  -6.27255062e-01f, -4.31683360e-02f, 5.66690342e-02f, 4.95893635e-01f, 
  2.37012116e-01f, 2.11167242e-01f, 1.53448358e-01f, -5.37585704e-01f, 
  -3.70781280e-01f, 9.21328737e-02f, 1.53827004e-01f, 7.05532466e-01f, 
  -4.73256124e-01f, 6.02309696e-01f, -5.19167487e-01f, 2.16303559e-01f, 
  1.58429050e+00f,}, 
  {-1.19188874e-01f, 7.42861062e-01f, -8.20213817e-01f, 6.04177393e-02f, 
  5.01386715e-01f, -7.93237338e-01f, 1.03063124e-01f, 3.19073045e-01f, 
  -7.45029316e-02f, -1.17764236e-01f, 2.15447116e-01f, -8.60175240e-02f, 
  -1.21831230e-01f, -1.03473977e-01f, -8.80365681e-02f, -5.75197875e-02f, 
  1.05394860e-01f, 1.00044119e-01f, 1.19628921e-01f, -2.97384932e-01f, 
  -8.92678592e-02f, -4.05102519e-01f, -2.39266206e-01f, -7.36400585e-02f, 
  -2.14271734e-01f, 1.10141051e-01f, -1.58967909e-01f, -1.94883433e-01f, 
  -3.87761426e-02f, -2.99182026e-01f, -1.18802621e-02f, -3.73908152e-01f, 
  -1.18483559e-01f,}, 
  {6.28846651e-01f, -2.09927122e-02f, -5.27983206e-02f, -1.25864510e-01f, 
  -2.73978962e-02f, -2.10457914e-01f, -3.32912056e-02f, 8.68132526e-02f, 
  -4.41501086e-03f, -4.61089958e-02f, -2.85265531e-01f, 8.65234671e-02f, 
  -2.43467010e-01f, -3.64874958e-02f, -2.13813210e-01f, -1.65865441e-01f, 
  -1.78282667e-01f, -2.78704233e-01f, -1.84008044e-01f, -1.78899753e-01f, 
  -2.01306345e-01f, -5.30263901e-02f, -1.06133020e-01f, -2.09872558e-01f, 
  -1.24675281e-01f, -4.66762971e-02f, 2.24280495e-01f, -6.86199214e-01f, 
  1.62985367e-01f, -1.01948854e-01f, 2.98757352e-01f, 1.13076945e-01f, 
  3.32750908e-01f,}, 
  {-2.95493748e-02f, 4.88408043e-01f, -1.34611286e-02f, 2.03976586e-01f, 
  4.20073168e-01f, -4.12522688e-02f, 4.09265766e-01f, 4.83385299e-01f, 
  1.27435194e-01f, -2.25362636e-01f, -7.20909180e-02f, -1.16565672e+00f, 
  5.15664319e-02f, 7.93091973e-02f, 3.20900733e-02f, 1.22984470e-01f, 
  -3.10720638e-02f, 8.84994058e-02f, -3.76852656e-01f, -5.04445866e-01f, 
  4.07947115e-02f, 5.42077389e-02f, -2.22477907e+00f, -1.25555339e-02f, 
  -5.63604969e-01f, 2.85043320e-01f, -1.74182667e+00f, -4.31633707e-01f, 
  -2.05348831e-01f, -6.46189163e-01f, 1.96839675e-01f, -5.52726461e-01f, 
  1.34763990e-02f,}, 
  {7.90217502e-02f, 9.68148522e-02f, 1.38677945e-01f, 2.21317302e-01f, 
  -2.69782580e-02f, 5.29203714e-01f, 1.37463401e-01f, 2.62573900e-01f, 
  3.61367126e-01f, 1.20996333e-01f, 1.07204236e-01f, 2.71118437e-01f, 
  5.95023288e-02f, 2.62735858e-01f, 3.89039881e-01f, 3.73091822e-01f, 
  2.78975030e-01f, 2.81569011e-01f, 3.17219794e-02f, 2.62451993e-01f, 
  4.11869933e-01f, 2.24465038e-01f, 1.65720407e-01f, 5.16192298e-01f, 
  6.00705505e-01f, 9.78633469e-02f, 3.32895053e-01f, 4.39084298e-01f, 
  4.55474941e-01f, 3.09833317e-02f, 2.58263301e-01f, 3.61877868e-01f, 
  1.29931486e-01f,}, 
  {-2.98600518e-01f, 1.12244725e-02f, -2.07441633e-01f, -8.73421182e-02f, 
  1.05569510e-01f, 2.81403438e-02f, 5.26245888e-02f, 2.67174423e-01f, 
  -1.77892826e-01f, -4.05829948e-02f, 4.06920482e-01f, 1.07934706e-01f, 
  1.12547924e-01f, -3.71715985e-02f, -6.28045441e-02f, -1.68400735e-01f, 
  -6.24372277e-02f, 1.69667066e-01f, 2.14265031e-01f, -3.06472591e-02f, 
  -1.62796017e-01f, 7.06237120e-02f, -4.92758550e-02f, -5.26185389e-01f, 
  1.52782317e-01f, -4.65785004e-02f, 9.42529275e-02f, -1.21254334e-01f, 
  -3.14490707e-01f, -1.77576162e-01f, -3.34038133e-01f, 1.15809107e-02f, 
  1.64503169e-01f,}, 
  {3.44835635e-01f, -1.58903875e-01f, -3.10316519e-02f, -4.93990274e-02f, 
  -1.60160660e-01f, -1.71758660e-02f, -1.69500604e-01f, -7.15141220e-01f, 
  -3.60036878e-02f, 7.79719358e-02f, 1.12501638e-01f, -1.80399481e-02f, 
  -2.89363093e-02f, 9.69054869e-02f, -2.27688184e-02f, 6.13992821e-01f, 
  1.64080501e-01f, -3.88676463e-02f, -2.68249103e-02f, -1.45989031e-02f, 
  5.29456836e-01f, 1.07699589e-01f, -2.20770664e-02f, -1.36433275e-01f, 
  2.28031764e-01f, -2.54038170e-01f, 1.12308941e-02f, 8.09533742e-01f, 
  3.93717303e-01f, -1.07163507e-01f, 1.49078030e-01f, 3.70204181e-01f, 
  4.09279333e-03f,}, 
  {-1.77440914e-01f, -3.01034573e-01f, 1.40191170e-02f, -4.72262362e-02f, 
  -3.44803357e-01f, 2.37280530e-01f, -1.32451825e-01f, -1.15947698e-01f, 
  3.33863457e-01f, 2.36090470e-01f, 1.04869132e-01f, 2.41153251e-01f, 
  1.32492592e-01f, 5.18705373e-02f, 7.03203392e-02f, 2.66290909e-01f, 
  3.96947048e-01f, 1.48562271e-01f, 2.40463391e-01f, 2.04836073e-01f, 
  1.88369696e-01f, 2.29061103e-01f, 2.46346191e-01f, -8.12412708e-02f, 
  2.04474298e-01f, 2.93040793e-01f, 1.10599003e-01f, 2.95805230e-01f, 
  -1.20071539e-02f, 2.22553905e-01f, 4.82319398e-01f, 5.79085909e-02f, 
  -5.35951582e-02f,}, 
  {-1.15630652e-01f, 2.01021233e-01f, 2.28745734e-01f, 4.48600585e-02f, 
  6.88345632e-01f, -1.07357302e-01f, -5.78515784e-01f, 4.46043254e-01f, 
  8.71647150e-02f, 2.33961355e-01f, -1.71729037e+00f, -2.10901515e-01f, 
  -5.14676570e-01f, 2.12365539e-01f, -5.72823195e-01f, -8.00144270e-03f, 
  1.97626877e-01f, 1.95208093e-04f, -6.18258480e-01f, -3.99193563e-01f, 
  -2.27240108e-01f, -2.51457570e-01f, -9.08213331e-02f, 6.09135165e-02f, 
  -2.38593185e-01f, 3.87119073e-02f, -2.44729470e-01f, -1.68443838e-01f, 
  2.04552768e-01f, -3.29747028e-01f, -3.24255250e-02f, -2.70463660e-01f, 
  -4.00431040e-01f,}, 
  {7.49894333e-01f, 3.23040205e-01f, 3.13039388e-02f, 2.06467248e-01f, 
  2.81038555e-01f, 4.91016489e-03f, 9.12325232e-02f, -3.61523839e+00f, 
  1.99124706e-01f, 9.56001308e-02f, 2.70332383e-01f, -2.30553627e-01f, 
  -4.49698926e-02f, -6.39703555e-02f, -1.43280302e-02f, -2.14191270e-01f, 
  2.28677874e-01f, 5.27037959e-02f, -2.89710517e-01f, -1.96457830e-01f, 
  9.97862090e-02f, -7.28108304e-02f, -1.59231462e-01f, -6.96894391e-01f, 
  -4.54835262e-01f, -1.66221784e+00f, -1.92981322e-01f, -3.78639128e-01f, 
  -1.41691241e-01f, -9.26647037e-02f, 2.67418790e-01f, -5.92122112e-01f, 
  -5.40072591e-01f,}, 
  {-1.81441959e-01f, -6.27508838e-02f, -1.28872790e-03f, 3.98884781e-02f, 
  -8.14669526e-02f, 3.14852327e-04f, -4.71735634e-02f, 7.33338771e-01f, 
  3.53501762e-02f, 7.76833782e-02f, 8.26870929e-02f, 7.64410169e-02f, 
  2.15836073e-02f, 1.47602579e-01f, -1.58747315e-02f, 6.93172662e-02f, 
  3.41401715e-01f, 2.98444073e-02f, 2.16729461e-01f, -4.46395330e-01f, 
  -8.45410780e-02f, 2.94424576e-02f, 6.31251549e-02f, 9.58091335e-02f, 
  7.83598233e-01f, 3.49369023e-01f, 6.71073750e-02f, -4.06574224e-01f, 
  2.88466405e-01f, -6.91082449e-01f, 2.86984286e-01f, 1.03988520e-01f, 
  -5.87542798e-01f,},
};
