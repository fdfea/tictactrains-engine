#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "debug.h"
#include "defs.h"
#include "types.h"
#include "util.h"
#include "vector.h"

#define CONFIG_FILENAME             "ttt.conf"
#define CONFIG_COMPUTER_PLAYING     "COMPUTER_PLAYING"
#define CONFIG_COMPUTER_PLAYER      "COMPUTER_PLAYER"
#define CONFIG_RULES_TYPE           "RULES_TYPE"
#define CONFIG_SIMULATIONS          "SIMULATIONS"
#define CONFIG_SCORING_ALGORITHM    "SCORING_ALGORITHM"
#define CONFIG_PREDICTION_POLICY    "PREDICTION_POLICY"
#define CONFIG_PREDICTION_STRATEGY  "PREDICTION_STRATEGY"
#define CONFIG_START_POSITION       "STARTING_POSITION"

#define CONFIG_MAXLINE              (128)
#define CONFIG_MAX_MOVES_STR_LEN    (ROWS*COLUMNS*2)

#define CONFIG_STRNCMP(pKey, Param) (strncmp(pKey, Param, sizeof(Param)) == 0)

static int config_parse_moves(tVector *pMoves, char *pMovesStr);

void config_init(tConfig *pConfig)
{
    pConfig->ComputerPlaying = false;
    pConfig->ComputerPlayer = false;
    rules_config_init(&pConfig->RulesConfig);
    mcts_config_init(&pConfig->MctsConfig);
    vector_init(&pConfig->StartPosition);
}

void config_free(tConfig *pConfig)
{
    for (tIndex i = 0; i < vector_size(&pConfig->StartPosition); ++i)
    {
        tIndex *pIndex = (tIndex *) vector_get(&pConfig->StartPosition, i);
        free(pIndex);
    }
    vector_free(&pConfig->StartPosition);
}

int config_load(tConfig *pConfig)
{
    int Res = 0, Line = 0, Val = 0;
    FILE *pFile = NULL;
    char Buf[CONFIG_MAXLINE];
    char *pKey = NULL, *pValue = NULL, *pEnd = NULL, *pS = NULL, *pD = NULL;
    
    struct 
    {
        bool ComputerPlaying, ComputerPlayer, RulesType, Simulations, 
             ScoringAlgorithm, PredictionPolicy, PredictionStrategy, StartPosition;
    } 
    Found = {false, false, false, false, false, false, false, false};

    if ((pFile = fopen(CONFIG_FILENAME, "r")) ISNOT NULL)
    {
        while (fgets(Buf, CONFIG_MAXLINE, pFile) ISNOT NULL)
        {
            Line++;

            //remove newline and spaces
            Buf[strcspn(Buf, "\n")] = '\0';

            pS = &Buf[0], pD = pS;
            do while (isspace(*pS)) pS++; while ((*pD++ = *pS++));

            //check for empty line or comment
            if (Buf[0] == '\0' OR Buf[0] == '#')
            {
                continue;
            }

            //handle configuration parameters
            pKey = strtok(Buf, "=");
            pValue = strtok(NULL, "=");

            if (pKey IS NULL OR pValue IS NULL)
            {
                Res = -EINVAL;
                goto Error;
            }

            if (NOT Found.StartPosition AND CONFIG_STRNCMP(pKey, CONFIG_START_POSITION))
            {
                Res = config_parse_moves(&pConfig->StartPosition, pValue);
                if (Res < 0)
                {
                    Res = -EINVAL;
                    goto Error;
                }
                Found.StartPosition = true;
                continue;
            }

            //convert value to number
            Val = strtol(pValue, &pEnd, 10);

            if (pValue == pEnd)
            {
                Res = -EINVAL;
                goto Error;
            }

            //compare keys to config params
            if (NOT Found.ComputerPlaying AND CONFIG_STRNCMP(pKey, CONFIG_COMPUTER_PLAYING))
            {
                if (Val == 0)
                {
                    pConfig->ComputerPlaying = false;
                }
                else if (Val == 1)
                {
                    pConfig->ComputerPlaying = true;
                }
                else
                {
                    Res = -EINVAL;
                    goto Error;
                }
                Found.ComputerPlaying = true;
            }
            else if (NOT Found.ComputerPlayer AND CONFIG_STRNCMP(pKey, CONFIG_COMPUTER_PLAYER))
            {
                if (Val == 0)
                {
                    pConfig->ComputerPlayer = false;
                }
                else if (Val == 1)
                {
                    pConfig->ComputerPlayer = true;
                }
                else
                {
                    Res = -EINVAL;
                    goto Error;
                }
                Found.ComputerPlayer = true;
            }
            else if (NOT Found.RulesType AND CONFIG_STRNCMP(pKey, CONFIG_RULES_TYPE))
            {
                switch (Val)
                {
                    case RULES_CLASSICAL:
                    {
                        pConfig->RulesConfig.RulesType = RULES_CLASSICAL;
                        break;
                    }
                    case RULES_MODERN: 
                    {
                        pConfig->RulesConfig.RulesType = RULES_MODERN;
                        break;
                    }
                    case RULES_EXPERIMENTAL: 
                    {
                        pConfig->RulesConfig.RulesType = RULES_EXPERIMENTAL;
                        break;
                    }
                    default: 
                    {
                        Res = -EINVAL;
                        goto Error;
                    }
                }
                Found.RulesType = true;
            }
            else if (NOT Found.Simulations AND CONFIG_STRNCMP(pKey, CONFIG_SIMULATIONS))
            {
                if (Val > 0 AND Val <= TVISITS_MAX)
                {
                    pConfig->MctsConfig.Simulations = Val;
                }
                else 
                {
                    Res = -EINVAL;
                    goto Error;
                }
                Found.Simulations = true;
            }
            else if (NOT Found.ScoringAlgorithm AND CONFIG_STRNCMP(pKey, CONFIG_SCORING_ALGORITHM))
            {
                switch (Val)
                {
                    case SCORING_ALGORITHM_OPTIMAL:
                    {
                        pConfig->MctsConfig.ScoringAlgorithm = SCORING_ALGORITHM_OPTIMAL;
                        break;
                    }
                    case SCORING_ALGORITHM_QUICK:
                    {
                        pConfig->MctsConfig.ScoringAlgorithm = SCORING_ALGORITHM_QUICK;
                        break;
                    }
                    default:
                    {
                        Res = -EINVAL;
                        goto Error;
                    }
                }
                Found.ScoringAlgorithm = true;
            }
            else if (NOT Found.PredictionPolicy AND CONFIG_STRNCMP(pKey, CONFIG_PREDICTION_POLICY))
            {
                switch (Val)
                {
                    case PREDICTION_POLICY_NEVER: 
                    {
                        pConfig->MctsConfig.PredictionPolicy = PREDICTION_POLICY_NEVER;
                        break;  
                    }
                    case PREDICTION_POLICY_ALWAYS:
                    {
                        pConfig->MctsConfig.PredictionPolicy = PREDICTION_POLICY_ALWAYS;
                        break;
                    }
                    case PREDICTION_POLICY_LONGPATHS:
                    {
                        pConfig->MctsConfig.PredictionPolicy = PREDICTION_POLICY_LONGPATHS;
                        break;
                    }
                    default: 
                    {
                        Res = -EINVAL;
                        goto Error;
                    }
                }
                Found.PredictionPolicy = true;
            }
            else if (NOT Found.PredictionStrategy AND CONFIG_STRNCMP(pKey, CONFIG_PREDICTION_STRATEGY))
            {
                switch (Val)
                {
                    case PREDICTION_STRATEGY_NBR_LINREG:
                    {
                        pConfig->MctsConfig.PredictionStrategy = PREDICTION_STRATEGY_NBR_LINREG;
                        break;  
                    }
                    case PREDICTION_STRATEGY_NBR_LOGREG:
                    {
                        pConfig->MctsConfig.PredictionStrategy = PREDICTION_STRATEGY_NBR_LOGREG;
                        break;
                    }
                    case PREDICTION_STRATEGY_SHP_MLP:
                    {
                        pConfig->MctsConfig.PredictionStrategy = PREDICTION_STRATEGY_SHP_MLP;
                        break;
                    }
                    default:
                    {
                        Res = -EINVAL;
                        goto Error;
                    }
                }
                Found.PredictionStrategy = true;
            }
            else
            {
                Res = -EINVAL;
                goto Error;
            }
        }
    }
    else
    {
        Res = -ENOENT;
        fprintf(stderr, "[ERROR] Could not open file \"%s\"\n", CONFIG_FILENAME);
        goto NoFile;
    }

    dbg_printf(DEBUG_INFO, "%s: %d\n", CONFIG_COMPUTER_PLAYING, pConfig->ComputerPlaying);
    dbg_printf(DEBUG_INFO, "%s: %d\n", CONFIG_COMPUTER_PLAYER, pConfig->ComputerPlayer);
    dbg_printf(DEBUG_INFO, "%s: %d\n", CONFIG_RULES_TYPE, pConfig->RulesConfig.RulesType);
    dbg_printf(DEBUG_INFO, "%s: %d\n", CONFIG_SIMULATIONS, pConfig->MctsConfig.Simulations);
    dbg_printf(DEBUG_INFO, "%s: %d\n", CONFIG_SCORING_ALGORITHM, pConfig->MctsConfig.ScoringAlgorithm);
    dbg_printf(DEBUG_INFO, "%s: %d\n", CONFIG_PREDICTION_POLICY, pConfig->MctsConfig.PredictionPolicy);
    dbg_printf(DEBUG_INFO, "%s: %d\n\n", CONFIG_PREDICTION_STRATEGY, pConfig->MctsConfig.PredictionStrategy);

    goto Success;

Error:
    fprintf(stderr, "[ERROR] Malformatted configuration at line %d\n", Line);

Success:
    fclose(pFile);

NoFile:
    return Res;
}

static int config_parse_moves(tVector *pMoves, char *pMovesStr)
{
    int Res = 0;
    char Move[BOARD_ID_STR_LEN] = {'\0'};
    tIndex Index = 0, MoveIndex = 0, *pMoveIndex = NULL;

    if (pMovesStr IS NULL)
    {
        Res = -EINVAL;
        goto Error;
    }

    while (Index < CONFIG_MAX_MOVES_STR_LEN)
    {
        Move[0] = pMovesStr[Index];
        if (Move[0] == '\0')
        {
            break;
        }

        Move[1] = pMovesStr[Index+1];
        if (Move[1] == '\0')
        {
            Res = -EINVAL;
            goto Error;
        }

        if (NOT board_id_valid(&Move))
        {
            Res = -EINVAL;
            goto Error;
        }
        
        MoveIndex = board_id_index(&Move);
        if (NOT board_index_valid(MoveIndex))
        {
            Res = -EINVAL;
            goto Error;
        }

        pMoveIndex = malloc(sizeof(tIndex));
        if (pMoveIndex IS NULL)
        {
            Res = -ENOMEM;
            goto Error;
        }

        *pMoveIndex = MoveIndex;
        vector_add(pMoves, pMoveIndex);
        
        Index += 2;
    }

    Error:
        return Res;
}
