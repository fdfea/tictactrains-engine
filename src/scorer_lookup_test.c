#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "random.h"
#include "rules.h"
#include "scorer.h"
#include "scorer_lookup_table.h"
#include "util.h"

int main(void)
{
    tRulesConfig RulesConfig;
    tRules Rules;
    tRandom Random;
    tBoard Board;

    printf("Initializing scorer lookup\n");
    scorer_init();
    printf("Initialized scorer lookup\n");

    rules_config_init(&RulesConfig);

    RulesConfig.RulesType = RULES_CLASSICAL;

    rules_init(&Rules, &RulesConfig);
    random_init(&Random);

    int Simulations = 100000;
    int Failures = 0;

    printf("Starting simulations\n");

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

    printf("Freeing scorer lookup\n");
    scorer_free();
    printf("Freed scorer lookup\n");

    return 0;
}
