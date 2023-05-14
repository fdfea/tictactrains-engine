#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "bitutil.h"
#include "board.h"
#include "random.h"
#include "rules.h"
#include "scorer.h"
#include "util.h"

#include "vector.h"

//upgrade gcc version?
//try using -Ofast?

#define AREA_3X4_INDICES 12
#define AREA_3X4_EXITS 7

extern int TotalPaths;

typedef struct Area3x4Path
{
    uint64_t Path;
    tSize Length;
    int Used;
}
tArea3x4Path;

typedef struct Area3x4PathLookup
{
    tVector Paths;
    tIndex Exit;
}
tArea3x4PathLookup;

typedef struct Area3x4IndexLookup
{
    tVector Exits;
    tSize LongestPath;
}
tArea3x4IndexLookup;

typedef struct Area3x4Lookup
{
    tArea3x4IndexLookup Indices[AREA_3X4_INDICES];
}
tArea3x4Lookup;

static double time_diff_ms(struct timespec *pBegin, struct timespec *pEnd);
static void area_3x4_print(uint16_t Data);
static void area_7x7_print(uint64_t Data);

int main(void)
{
    /*
    uint16_t Data = 0b111100001111;
    uint16_t Area = 0U;

    area_3x4_area(Data, 8, &Area);

    printf("DATA\n");
    area_3x4_print(Data);
    printf("AREA\n");
    area_3x4_print(Area);
    */

    /*
    uint16_t Data = 0b011001111110;

    printf("DATA\n");
    area_3x4_print(Data);

    tArea3x4Lookup Lookup;
    tArea3x4Lookup *pLookup = &Lookup;
    memset(pLookup, 0, sizeof(tArea3x4Lookup));

    for (tIndex Start = 0; Start < AREA_3X4_INDICES; ++Start)
    {
        tArea3x4IndexLookup *pIndexLookup = &pLookup->Indices[Start];

        if (BitTest16(Data, Start))
        {
            vector_init(&pIndexLookup->Exits);

            pIndexLookup->LongestPath = area_3x4_index_longest_path(Data, Start);

            for (tIndex Exit = 0; Exit < AREA_3X4_EXITS; ++Exit)
            {
                tIndex End = AREA_3X4_EXIT_INDEX(Exit);
                uint16_t Path = 0U;
                tSize Length;

                if (BitTest16(Data, End) AND area_3x4_longest_path(Data, Start, End, &Length, &Path))
                {
                    tArea3x4PathLookup *pPathLookup = emalloc(sizeof(tArea3x4PathLookup));
                    uint16_t Used = 0U;

                    vector_init(&pPathLookup->Paths);
                    area_3x4_get_lookup_paths(Data, Start, End, 0U, &pPathLookup->Paths, &Used);

                    pPathLookup->Exit = Exit;

                    tVectorIterator PathsIterator;

                    vector_iterator_init(&PathsIterator, &pPathLookup->Paths);

                    bool DuplicatePath = false;

                    while (vector_iterator_has_next(&PathsIterator))
                    {
                        tArea3x4Path *pPath = vector_iterator_next(&PathsIterator);

                        if (pPath->Path == Path)
                        {
                            DuplicatePath = true;
                            break;
                        }
                    }

                    if (NOT DuplicatePath)
                    {
                        tArea3x4Path *pPath;

                        pPath = emalloc(sizeof(tArea3x4Path));
                        pPath->Path = Path;
                        pPath->Length = Length;

                        vector_add(&pPathLookup->Paths, (void *) pPath);
                    }

                    vector_add(&pIndexLookup->Exits, (void *) pPathLookup);

                    printf("=========== start: %d, exit: %d, end: %d, max length: %d ===========\n", Start, Exit, End, Length);
                    printf("DATA\n");
                    area_3x4_print(Data);
                    printf("FOUND PATHS: %d\n", vector_size(&pPathLookup->Paths));
                    vector_iterator_init(&PathsIterator, &pPathLookup->Paths);
                    while (vector_iterator_has_next(&PathsIterator))
                    {
                        tArea3x4Path *pPath = vector_iterator_next(&PathsIterator);
                        if (NOT BitEmpty16(pPath->Path & ~Data))
                        {
                            printf("bad path!!!\n");
                        }
                        printf("length: %d, path:\n", pPath->Length);
                        area_3x4_print(pPath->Path);
                    }
                }
            }
        }
    }
    */

    /*
    //uint16_t Data = 0b010111011111;
    //tIndex Start = 10;
    //tIndex End = 8;

    uint16_t Data = 0b001111110001;
    tIndex Start = 0;
    tIndex End = 7;
    tVector Paths;

    vector_init(&Paths);
    area_3x4_get_lookup_paths(Data, Start, End, 0U, &Paths);

    tVectorIterator PathsIterator;
    vector_iterator_init(&PathsIterator, &Paths);

    area_3x4_print(Data);

    printf("PATHS\n");
    while (vector_iterator_has_next(&PathsIterator))
    {
        tArea3x4Path *pPath = vector_iterator_next(&PathsIterator);
        printf("length: %d, path:\n", pPath->Length);
        area_3x4_print(pPath->Path);
    }
    printf("DONE\n");

    uint16_t Path = 0U;
    tSize Length;
    bool Found = area_3x4_longest_path(Data, Start, End, &Length, &Path);

    printf("found: %d, length: %d, longest path:\n", Found, Length);
    area_3x4_print(Path);

    vector_free(&Paths);
    */

    tRulesConfig RulesConfig;
    tRules Rules;
    tRandom Random;
    tBoard Board;

    printf("initializing scorer lookup\n");
    scorer_init();
    printf("initialized scorer lookup\n");

    printf("TOTAL PATHS: %d\n", TotalPaths);

    //goto Done;

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

    goto Done;

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

Done:
    printf("freeing scorer lookup\n");
    scorer_free();
    printf("freed scorer lookup\n");

    return 0;
}

static double time_diff_ms(struct timespec *pBegin, struct timespec *pEnd)
{
    return (pEnd->tv_sec * 1.0e3 + pEnd->tv_nsec / 1.0e6) - (pBegin->tv_sec * 1.0e3 + pBegin->tv_nsec / 1.0e6);
}

static void area_3x4_print(uint16_t Data)
{
    for (tIndex i = 0; i < AREA_3X4_INDICES; ++i)
    {
        printf("[%c]", BitTest16(Data, i) ? 'X' : ' ');
        if ((i + 1) % 4 == 0)
            printf("\n");
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
