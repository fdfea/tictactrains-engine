#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "random.h"
#include "rules.h"
#include "scorer.h"

static void board_compare_score(uint64_t Data);

int main(void)
{
    tRulesConfig RulesConfig;
    tRules Rules;
    tRandom Random;
    tBoard Board;

    rules_config_init(&RulesConfig);

    RulesConfig.RulesType = RULES_CLASSICAL;

    rules_init(&Rules, &RulesConfig);
    random_init(&Random);

    //rules_simulate_playout(&Rules, &Board, &Random, true);

    //board_compare_score(0x0000A487142BE5FDULL);
    //board_compare_score(0x0000C37009D7C7EAULL);
    //board_compare_score(0x00004DEA8BCBC782ULL);

    int Simulations = 10000;
    int Failures = 0;

    for (int i = 0; i < Simulations; ++i)
    {
        board_init(&Board);
        rules_simulate_playout(&Rules, &Board, &Random, true);

        tScore OptimalScore = board_score(&Board, SCORING_ALGORITHM_OPTIMAL);
        tScore LookupScore = score(&Board);

        if (OptimalScore != LookupScore)
        {
            Failures++;

            char *Str = board_string(&Board);

            printf("========= scoring mismatch =========\n");
            printf("board:\n%s\n", Str);
            printf("naive score: %d\n", OptimalScore);
            printf("lookup score: %d\n", LookupScore);

            free(Str);
        }
    }

    printf("simulations: %d, failures: %d\n", Simulations, Failures);

    return 0;
}

static void board_compare_score(uint64_t Data)
{
    tBoard Board;

    board_init(&Board);

    Board.Data = Data;
    Board.Empty = 0ULL;

    char *Str = board_string(&Board);

    printf("board:\n%s\n", Str);
    printf("naive score: %d\n", board_score(&Board, SCORING_ALGORITHM_OPTIMAL));
    printf("lookup score: %d\n", score(&Board));

    free(Str);
}
