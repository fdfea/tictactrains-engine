/*
#include <stdio.h>
#include <stdlib.h>

#include "bitutil.h"
#include "board.h"
#include "random.h"
#include "rules.h"
#include "scorer.h"
#include "util.h"

int main(void)
{
    bool array1[ROWS*COLUMNS] = {
        1, 1, 1, 1, 0, 0, 0,
        1, 1, 1, 1, 0, 0, 0,
        1, 1, 1, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
    };

    bool array2[ROWS*COLUMNS] = {
        0, 0, 0, 0, 1, 1, 1,
        0, 0, 0, 0, 1, 1, 1,
        0, 0, 0, 0, 1, 1, 1,
        0, 0, 0, 0, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
    };

    bool array3[ROWS*COLUMNS] = {
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 1, 1, 1, 1,
        0, 0, 0, 1, 1, 1, 1,
        0, 0, 0, 1, 1, 1, 1,
    };

    bool array4[ROWS*COLUMNS] = {
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 0, 0, 0, 0,
        1, 1, 1, 0, 0, 0, 0,
        1, 1, 1, 0, 0, 0, 0,
        1, 1, 1, 0, 0, 0, 0,
    };

    uint64_t q1_mask = 0ULL;
    for (int i = 0; i < ROWS*COLUMNS; ++i) {
        if (array1[i]) BitSet64(&q1_mask, i);
    }
    printf("q1_mask: 0x%016I64XULL\n", q1_mask);

    uint64_t q2_mask = 0ULL;
    for (int i = 0; i < ROWS*COLUMNS; ++i) {
        if (array2[i]) BitSet64(&q2_mask, i);
    }
    printf("q2_mask: 0x%016I64XULL\n", q2_mask);

    uint64_t q3_mask = 0ULL;
    for (int i = 0; i < ROWS*COLUMNS; ++i) {
        if (array3[i]) BitSet64(&q3_mask, i);
    }
    printf("q3_mask: 0x%016I64XULL\n", q3_mask);

    uint64_t q4_mask = 0ULL;
    for (int i = 0; i < ROWS*COLUMNS; ++i) {
        if (array4[i]) BitSet64(&q4_mask, i);
    }
    printf("q4_mask: 0x%016I64XULL\n", q4_mask);

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

        tScore OptimalScore = board_score(&Board);
        tScore LookupScore = scorer_score(&Board);

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
*/
