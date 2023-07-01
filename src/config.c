#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "config.h"
#include "debug.h"
#include "rules.h"
#include "types.h"
#include "util.h"
#include "vector.h"

#define CONFIG_FILENAME                 "ttt.conf"
#define CONFIG_COMPUTER_PLAYING         "COMPUTER_PLAYING"
#define CONFIG_COMPUTER_PLAYER          "COMPUTER_PLAYER"
#define CONFIG_RULES_TYPE               "RULES_TYPE"
#define CONFIG_SIMULATIONS              "SIMULATIONS"
#define CONFIG_SEARCH_ONLY_NEIGHBORS    "SEARCH_ONLY_NEIGHBORS"
#define CONFIG_STARTING_MOVES           "STARTING_MOVES"

#define CONFIG_MAXLINE              128
#define CONFIG_MAX_MOVES_STR_LEN    (ROWS*COLUMNS*2)

#define CONFIG_STRNCMP(pKey, Param) (strncmp(pKey, Param, sizeof(Param)) == 0)

static int config_parse_moves(tVector *pMoves, char *pMovesStr);

int config_init(tConfig *pConfig)
{
    int Res = 0;

    pConfig->ComputerPlaying = false;
    pConfig->ComputerPlayer = false;

    rules_config_init(&pConfig->RulesConfig);
    mcts_config_init(&pConfig->MctsConfig);

    Res = vector_init(&pConfig->StartingMoves);

    return Res;
}

void config_free(tConfig *pConfig)
{
    for (tIndex i = 0; i < vector_size(&pConfig->StartingMoves); ++i)
    {
        free(vector_get(&pConfig->StartingMoves, i));
    }

    vector_free(&pConfig->StartingMoves);
}

int config_load(tConfig *pConfig)
{
    int Res = 0, Line = 0, Val = 0;
    FILE *pFile = NULL;
    char Buf[CONFIG_MAXLINE];
    char *pKey = NULL, *pValue = NULL, *pEnd = NULL, *pS = NULL, *pD = NULL;
    
    struct
    {
        bool ComputerPlaying, ComputerPlayer, RulesType, Simulations, SearchOnlyNeighbors, StartPosition;
    }
    Found = { false, false, false, false, false, false };

    if ((pFile = fopen(CONFIG_FILENAME, "r")) ISNOT NULL)
    {
        while (fgets(Buf, CONFIG_MAXLINE, pFile) ISNOT NULL)
        {
            Line++;

            Buf[strcspn(Buf, "\n")] = '\0';

            pS = &Buf[0], pD = pS;
            do while (isspace(*pS)) pS++; while ((*pD++ = *pS++));

            if (Buf[0] == '\0' OR Buf[0] == '#')
            {
                continue;
            }

            pKey = strtok(Buf, "=");
            pValue = strtok(NULL, "=");

            if (pKey IS NULL OR pValue IS NULL)
            {
                Res = -EINVAL;
                goto Error;
            }

            if (NOT Found.StartPosition AND CONFIG_STRNCMP(pKey, CONFIG_STARTING_MOVES))
            {
                Res = config_parse_moves(&pConfig->StartingMoves, pValue);
                if (Res < 0)
                {
                    Res = -EINVAL;
                    goto Error;
                }

                Found.StartPosition = true;
                continue;
            }

            Val = strtol(pValue, &pEnd, 10);

            if (pValue == pEnd)
            {
                Res = -EINVAL;
                goto Error;
            }

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
            else if (NOT Found.SearchOnlyNeighbors AND CONFIG_STRNCMP(pKey, CONFIG_SEARCH_ONLY_NEIGHBORS))
            {
                if (Val == 0)
                {
                    pConfig->MctsConfig.SearchOnlyNeighbors = false;
                }
                else if (Val == 1)
                {
                    pConfig->MctsConfig.SearchOnlyNeighbors = true;
                }
                else
                {
                    Res = -EINVAL;
                    goto Error;
                }

                Found.SearchOnlyNeighbors = true;
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
        fprintf(stderr, "[ERROR] Cannot open file \"%s\"\n", CONFIG_FILENAME);
        goto NoFile;
    }

    dbg_printf(DEBUG_LEVEL_INFO, "%s: %d", CONFIG_COMPUTER_PLAYING, pConfig->ComputerPlaying);
    dbg_printf(DEBUG_LEVEL_INFO, "%s: %d", CONFIG_COMPUTER_PLAYER, pConfig->ComputerPlayer);
    dbg_printf(DEBUG_LEVEL_INFO, "%s: %d", CONFIG_RULES_TYPE, pConfig->RulesConfig.RulesType);
    dbg_printf(DEBUG_LEVEL_INFO, "%s: %d", CONFIG_SIMULATIONS, pConfig->MctsConfig.Simulations);
    dbg_printf(DEBUG_LEVEL_INFO, "%s: %d", CONFIG_SEARCH_ONLY_NEIGHBORS, pConfig->MctsConfig.SearchOnlyNeighbors);

    goto Success;

Error:
    fprintf(stderr, "[ERROR] Cannot parse configuration at line %d\n", Line);

Success:
    fclose(pFile);

NoFile:
    return Res;
}

static int config_parse_moves(tVector *pMoves, char *pMovesStr)
{
    int Res = 0;
    char Move[BOARD_ID_STR_LEN] = { 0 };
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

        pMoveIndex = emalloc(sizeof(tIndex));
        *pMoveIndex = MoveIndex;

        vector_add(pMoves, pMoveIndex);
        
        Index += 2;
    }

Error:
    return Res;
}
