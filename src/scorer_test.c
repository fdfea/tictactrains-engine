/*
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bitutil.h"
#include "board.h"
#include "random.h"
#include "rules.h"
#include "scorer.h"
#include "util.h"

static double time_diff_ms(struct timespec *pBegin, struct timespec *pEnd);

int main(void)
{
    tRulesConfig RulesConfig;
    tRules Rules;
    tRandom Random;
    tBoard Board;

    printf("initializing scorer lookup\n");
    scorer_init();
    printf("initialized scorer lookup\n");

    rules_config_init(&RulesConfig);

    RulesConfig.RulesType = RULES_CLASSICAL;

    rules_init(&Rules, &RulesConfig);
    random_init(&Random);

    int Simulations = 100000;
    int Failures = 0;

    printf("starting simulations\n");

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

    printf("starting benchmarks\n");

    printf("BOARD SCORE\n");

    struct timespec Begin1, End1;
    clock_gettime(CLOCK_REALTIME, &Begin1);

    for (int i = 0; i < Simulations; ++i)
    {
        board_init(&Board);
        rules_simulate_playout(&Rules, &Board, &Random, true);
        board_score(&Board);
    }

    clock_gettime(CLOCK_REALTIME, &End1);
    printf("simulations: %d, time elapsed: %.3lf ms\n", Simulations, time_diff_ms(&Begin1, &End1));

    printf("SCORER SCORE\n");

    struct timespec Begin2, End2;
    clock_gettime(CLOCK_REALTIME, &Begin2);

    for (int i = 0; i < Simulations; ++i)
    {
        board_init(&Board);
        rules_simulate_playout(&Rules, &Board, &Random, true);
        scorer_score(&Board);
    }

    clock_gettime(CLOCK_REALTIME, &End2);
    printf("simulations: %d, time elapsed: %.3lf ms\n", Simulations, time_diff_ms(&Begin2, &End2));


    printf("freeing scorer lookup\n");
    scorer_free();
    printf("freed scorer lookup\n");

    return 0;
}

static double time_diff_ms(struct timespec *pBegin, struct timespec *pEnd)
{
    return (pEnd->tv_sec * 1.0e3 + pEnd->tv_nsec / 1.0e6) - (pBegin->tv_sec * 1.0e3 + pBegin->tv_nsec / 1.0e6);
}
*/

/*
static void area_3x4_print(uint16_t Data)
{
    for (tIndex i = 0; i < AREA_3X4_INDICES; ++i)
    {
        printf("[%c]", BitTest16(Data, i) ? 'X' : ' ');
        if ((i + 1) % 4 == 0) printf("\n");
    }
    printf("\n");
}

static void area_7x7_print(uint64_t Data)
{
    tBoard Board;

    board_init(&Board);

    Board.Data = Data;
    Board.Empty = 0ULL;

    char *Str = board_string(&Board);

    printf("%s\n\n", Str);

    free(Str);
}
*/
