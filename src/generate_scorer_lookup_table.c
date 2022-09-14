#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "scorer_lookup_table.h"
#include "types.h"
#include "util.h"

typedef struct Area3x4AjacentIndexLookup
{
    bool LeftValid, RightValid, TopValid, BottomValid;
    tIndex Left, Right, Top, Bottom;
}
tArea3x4AdjacentIndexLookup;

static const tArea3x4AdjacentIndexLookup Area3x4AdjacentIndexLookup[AREA_3X4_INDICES] = {
    { false, true , false, true ,  0,  1,  0,  4 },
    { true , true , false, true ,  0,  2,  0,  5 },
    { true , true , false, true ,  1,  3,  0,  6 },
    { true , false, false, true ,  2,  0,  0,  7 },
    { false, true , true , true ,  0,  5,  0,  8 },
    { true , true , true , true ,  4,  6,  1,  9 },
    { true , true , true , true ,  5,  7,  2, 10 },
    { true , false, true , true ,  6,  0,  3, 11 },
    { false, true , true , false,  0,  9,  4,  0 },
    { true , true , true , false,  8, 10,  5,  0 },
    { true , true , true , false,  9, 11,  6,  0 },
    { true , false, true , false, 10,  0,  7,  0 },
};

#define LEFT_VALID(i)       Area3x4AdjacentIndexLookup[i].LeftValid
#define RIGHT_VALID(i)      Area3x4AdjacentIndexLookup[i].RightValid
#define TOP_VALID(i)        Area3x4AdjacentIndexLookup[i].TopValid
#define BOTTOM_VALID(i)     Area3x4AdjacentIndexLookup[i].BottomValid

#define LEFT(i)         Area3x4AdjacentIndexLookup[i].Left
#define RIGHT(i)        Area3x4AdjacentIndexLookup[i].Right
#define TOP(i)          Area3x4AdjacentIndexLookup[i].Top
#define BOTTOM(i)       Area3x4AdjacentIndexLookup[i].Bottom

static const tIndex Area3x4ExitIndexLookup[AREA_3X4_EXITS] = { 3, 7, 11, 8, 9, 10, 11 };
static const bool Area3x4ExitLookup[AREA_3X4_INDICES] = { false, false, false, true, false, false, false, true, true, true, true, true };
static const bool Area3x4RightExitLookup[AREA_3X4_EXITS] = { true, true, true, false, false, false, false };

#define AREA_3X4_EXIT_INDEX(i)      Area3x4ExitIndexLookup[i]
#define AREA_3X4_EXIT(i)            Area3x4ExitLookup[i]
#define AREA_3X4_RIGHT_EXIT(i)      Area3x4RightExitLookup[i]
#define AREA_3x4_BOTTOM_EXIT(i)     (NOT AREA_3X4_RIGHT_EXIT(i))

bool BitEmpty16(uint16_t Bits);
bool BitTest16(uint16_t Bits, tIndex Index);
void BitSet16(uint16_t *pBits, tIndex Index);
void BitReset16(uint16_t *pBits, tIndex Index);

static void area_3x4_lookup_init(tArea3x4Lookup *pAreaLookup, uint16_t Data);
static void area_3x4_lookup_print(tArea3x4Lookup *pAreaLookup);
static void area_3x4_print(uint16_t Data);

static tSize area_3x4_index_longest_path(uint16_t Data, tIndex Index);
static bool area_3x4_longest_checked_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath);
static uint16_t area_3x4_shortest_checked_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength);
static bool area_3x4_least_exit_longest_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath, tSize *pExits);

static bool area_3x4_directive_checked_path(uint16_t Data, tIndex Start, tIndex End, uint16_t Checked, tSize *pLength, uint16_t *pPath);
static bool area_3x4_left_up_path(uint16_t Data, tIndex Start, tIndex End, uint16_t Checked, tSize *pLength, uint16_t *pPath);
static bool area_3x4_left_down_path(uint16_t Data, tIndex Start, tIndex End, uint16_t Checked, tSize *pLength, uint16_t *pPath);
static bool area_3x4_right_up_path(uint16_t Data, tIndex Start, tIndex End, uint16_t Checked, tSize *pLength, uint16_t *pPath);
static bool area_3x4_right_down_path(uint16_t Data, tIndex Start, tIndex End, uint16_t Checked, tSize *pLength, uint16_t *pPath);

int main(void)
{
    /*
    uint16_t Data = 0x0999U;
    tArea3x4Lookup AreaLookup;

    printf("data:\n");
    area_3x4_print(Data);

    tSize Len1, Len2;
    uint16_t Path1, Path2;

    tIndex Start = 0, End = 11;

    area_3x4_longest_checked_path(Data, Start, End, &Len1, &Path1);
    Path2 = area_3x4_shortest_checked_path(Data, Start, End, &Len2);

    printf("longest path: %d\n", Len1);
    area_3x4_print(Path1);
    printf("shortest path: %d\n", Len2);
    area_3x4_print(Path2);
    */

    //printf("init:\n");
    //area_3x4_lookup_init(&AreaLookup, Data);
    //printf("lookup:\n");
    //area_3x4_lookup_print(&AreaLookup);

    for (uint16_t Data = 0U; Data < AREA_3X4_LOOKUP_SIZE; ++Data)
    {
        tArea3x4Lookup AreaLookup;
        
        area_3x4_lookup_init(&AreaLookup, Data);
        area_3x4_lookup_print(&AreaLookup);
    }

    return 0;
}

static void area_3x4_lookup_init(tArea3x4Lookup *pAreaLookup, uint16_t Data)
{
    for (tIndex x = 0; x < AREA_3X4_INDICES; ++x)
    {
        uint16_t LongestIndexPathLength = 0;

        if (BitTest16(Data, x))
        {
            LongestIndexPathLength = area_3x4_index_longest_path(Data, x);
            //printf("longest index path length, %d: %d\n", x, LongestIndexPathLength);
        }

        pAreaLookup->IndexPaths[x].LongestPath = LongestIndexPathLength;

        for (tIndex y = 0; y < AREA_3X4_EXITS; ++y)
        {
            bool LongestExitPathFound = false, LeastExitPathFound = false, ShortestPathFound = false;
            tIndex ExitIndex = AREA_3X4_EXIT_INDEX(y);
            tSize LongestExitPathLength = 0, LeastExitPathLength = 0, ShortestPathLength = 0;
            uint16_t LongestExitPath = 0U, LeastExitPath = 0U, ShortestPath = 0U;
        
            if (BitTest16(Data, x) AND BitTest16(Data, ExitIndex))
            {
                //if longest and left are same length, just use left?
                LongestExitPathFound = area_3x4_longest_checked_path(Data, x, ExitIndex, &LongestExitPathLength, &LongestExitPath);
                //DirectiveExitPathFound = area_3x4_directive_checked_path(Data, x, y, 0U, &DirectiveExitPathLength, &DirectiveExitPath);
                tSize PathExits;
                LeastExitPathFound = area_3x4_least_exit_longest_path(Data, x, ExitIndex, &LeastExitPathLength, &LeastExitPath, &PathExits);

                ShortestPath = area_3x4_shortest_checked_path(Data, x, ExitIndex, &ShortestPathLength);
                ShortestPathFound = NOT BitEmpty16(ShortestPath);
            }

            if (LongestExitPathFound AND LeastExitPathFound AND ShortestPathFound)
            {
                pAreaLookup->IndexPaths[x].ExitPaths[y] = (tArea3x4PathLookup) { LongestExitPath, LeastExitPath, ShortestPath }; 

                //printf("longest exit path, %d -> %d:\n", x, y);
                //area_3x4_print(LongestExitPath);
                //printf("left exit path, %d -> %d:\n", x, y);
                //area_3x4_print(LeftExitPath);
            }
            else
            {
                pAreaLookup->IndexPaths[x].ExitPaths[y] = (tArea3x4PathLookup) { 0U, 0U, 0U }; 
            }

            if ((LongestExitPathFound != LeastExitPathFound) OR (LongestExitPathFound != ShortestPathFound))
            {
                printf("something weird happened...\n");
            }
        }
    }
}

static void area_3x4_lookup_print(tArea3x4Lookup *pAreaLookup)
{
    printf("{ {\n");
    for (tIndex x = 0; x < AREA_3X4_INDICES; ++x)
    {
        printf("    { { ");
        for (tIndex y = 0; y < AREA_3X4_EXITS; ++y)
        {
            char *Format = IF (y + 1 == AREA_3X4_EXITS) THEN "{0x%04xU, 0x%04xU, 0x%04xU}" ELSE "{0x%04xU, 0x%04xU, 0x%04xU}, ";
            printf(Format,
                pAreaLookup->IndexPaths[x].ExitPaths[y].LongestPath,
                pAreaLookup->IndexPaths[x].ExitPaths[y].LeastExitPath,
                pAreaLookup->IndexPaths[x].ExitPaths[y].ShortestPath
            );
        }
        printf(" }, %d },\n", pAreaLookup->IndexPaths[x].LongestPath);
    }
    printf("} },\n");
}

static void area_3x4_print(uint16_t Data)
{
    for (tIndex i = 0; i < AREA_3X4_INDICES; ++i)
    {
        char *Format = IF ((i+1) % AREA_3X4_COLUMNS == 0) THEN "[%c]\n" ELSE "[%c]";
        char C = IF (BitTest16(Data, i)) THEN 'X' ELSE ' ';

        printf(Format, C);
    }
}

static tSize area_3x4_index_longest_path(uint16_t Data, tIndex Index)
{
    tSize PathLength, MaxPathLength = 0;

    BitReset16(&Data, Index);

    if (LEFT_VALID(Index) AND BitTest16(Data, LEFT(Index)))
    {
        PathLength = area_3x4_index_longest_path(Data, LEFT(Index));
        SET_IF_GREATER(PathLength, MaxPathLength);
    }

    if (RIGHT_VALID(Index) AND BitTest16(Data, RIGHT(Index)))
    {
        PathLength = area_3x4_index_longest_path(Data, RIGHT(Index));
        SET_IF_GREATER(PathLength, MaxPathLength);
    }

    if (TOP_VALID(Index) AND BitTest16(Data, TOP(Index)))
    {
        PathLength = area_3x4_index_longest_path(Data, TOP(Index));
        SET_IF_GREATER(PathLength, MaxPathLength);
    }

    if (BOTTOM_VALID(Index) AND BitTest16(Data, BOTTOM(Index)))
    {
        PathLength = area_3x4_index_longest_path(Data, BOTTOM(Index));
        SET_IF_GREATER(PathLength, MaxPathLength);
    }

    return MaxPathLength + 1;
}

static bool area_3x4_longest_checked_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath)
{
    tSize PathLength, MaxPathLength = 0;
    uint16_t MaxPath, ThisPath = 0U;
    bool PathFound = false;

    BitReset16(&Data, Start);
    BitSet16(&ThisPath, Start);

    MaxPath = ThisPath;

    if (Start == End)
    {
        PathFound = true;
        goto Done;
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_longest_checked_path(Data, LEFT(Start), End, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
        }
    }

    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_longest_checked_path(Data, RIGHT(Start), End, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
        }
    }

    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_longest_checked_path(Data, TOP(Start), End, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
        }
    }

    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_longest_checked_path(Data, BOTTOM(Start), End, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
        }
    }

Done:
    *pPath |= MaxPath;
    *pLength = MaxPathLength + 1;

    return PathFound;
}

static uint16_t area_3x4_shortest_checked_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength)
{
    tSize PathLength, MinPathLength = TSIZE_MAX;
    uint16_t Path, MinPath = 0U;
    bool PathFound = false;

    BitReset16(&Data, Start);

    if (Start == End)
    {
        PathFound = true;
        MinPathLength = 0;
        goto Done;
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start)))
    {
        if ((Path = area_3x4_shortest_checked_path(Data, LEFT(Start), End, &PathLength)))
        {
            if (PathLength < MinPathLength)
            {
                PathFound = true;
                MinPathLength = PathLength;
                MinPath = Path;
            }
        }
    }

    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start)))
    {
        if ((Path = area_3x4_shortest_checked_path(Data, RIGHT(Start), End, &PathLength)))
        {
            if (PathLength < MinPathLength)
            {
                PathFound = true;
                MinPathLength = PathLength;
                MinPath = Path;
            }
        }
    }

    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start)))
    {
        if ((Path = area_3x4_shortest_checked_path(Data, TOP(Start), End, &PathLength)))
        {
            if (PathLength < MinPathLength)
            {
                PathFound = true;
                MinPathLength = PathLength;
                MinPath = Path;
            }
        }
    }

    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start)))
    {
        if ((Path = area_3x4_shortest_checked_path(Data, BOTTOM(Start), End, &PathLength)))
        {
            if (PathLength < MinPathLength)
            {
                PathFound = true;
                MinPathLength = PathLength;
                MinPath = Path;
            }
        }
    }

Done:
    *pLength = MinPathLength + 1;

    if (PathFound) BitSet16(&MinPath, Start);

    return MinPath;
}

static bool area_3x4_least_exit_longest_path(uint16_t Data, tIndex Start, tIndex End, tSize *pLength, uint16_t *pPath, tSize *pExits)
{
    tSize PathLength, PathExits, MaxPathLength = 0, MinPathExits = TSIZE_MAX;
    uint16_t LeastExitLongestPath, ThisPath = 0U;
    bool PathFound = false;

    BitReset16(&Data, Start);
    BitSet16(&ThisPath, Start);

    LeastExitLongestPath = ThisPath;

    if (Start == End)
    {
        PathFound = true;
        MinPathExits = 0;
        goto Done;
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_least_exit_longest_path(Data, LEFT(Start), End, &PathLength, &Path, &PathExits))
        {
            PathFound = true;
            if (PathExits < MinPathExits)
            {
                MinPathExits = PathExits;
                MaxPathLength = PathLength;
                LeastExitLongestPath = Path;
            }
            else if (PathExits == MinPathExits AND PathLength > MaxPathLength)
            {
                MaxPathLength = PathLength;
                LeastExitLongestPath = Path;
            }
        }
    }

    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_least_exit_longest_path(Data, RIGHT(Start), End, &PathLength, &Path, &PathExits))
        {
            PathFound = true;
            if (PathExits < MinPathExits)
            {
                MinPathExits = PathExits;
                MaxPathLength = PathLength;
                LeastExitLongestPath = Path;
            }
            else if (PathExits == MinPathExits AND PathLength > MaxPathLength)
            {
                MaxPathLength = PathLength;
                LeastExitLongestPath = Path;
            }
        }
    }

    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_least_exit_longest_path(Data, TOP(Start), End, &PathLength, &Path, &PathExits))
        {
            PathFound = true;
            if (PathExits < MinPathExits)
            {
                MinPathExits = PathExits;
                MaxPathLength = PathLength;
                LeastExitLongestPath = Path;
            }
            else if (PathExits == MinPathExits AND PathLength > MaxPathLength)
            {
                MaxPathLength = PathLength;
                LeastExitLongestPath = Path;
            }
        }
    }

    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_least_exit_longest_path(Data, BOTTOM(Start), End, &PathLength, &Path, &PathExits))
        {
            PathFound = true;
            if (PathExits < MinPathExits)
            {
                MinPathExits = PathExits;
                MaxPathLength = PathLength;
                LeastExitLongestPath = Path;
            }
            else if (PathExits == MinPathExits AND PathLength > MaxPathLength)
            {
                MaxPathLength = PathLength;
                LeastExitLongestPath = Path;
            }
        }
    }

Done:
    *pPath |= LeastExitLongestPath;
    *pLength = MaxPathLength + 1;
    *pExits = IF AREA_3X4_EXIT(Start) THEN MinPathExits + 1 ELSE MinPathExits;

    return PathFound;
}

//might need to do up-right, down-right path sometimes?
static bool area_3x4_directive_checked_path(uint16_t Data, tIndex Start, tIndex End, uint16_t Checked, tSize *pLength, uint16_t *pPath)
{
    bool PathFound;

    if (AREA_3X4_RIGHT_EXIT(End))
    {
        PathFound = area_3x4_left_up_path(Data, Start, AREA_3X4_EXIT_INDEX(End), Checked, pLength, pPath);
    }
    else
    {
        PathFound = area_3x4_left_down_path(Data, Start, AREA_3X4_EXIT_INDEX(End), Checked, pLength, pPath);
    }

    return PathFound;
}

static bool area_3x4_left_up_path(uint16_t Data, tIndex Start, tIndex End, uint16_t Checked, tSize *pLength, uint16_t *pPath)
{
    tSize PathLength, MaxPathLength = 0;
    uint16_t MaxPath, ThisPath = 0U;
    bool PathFound = false;

    BitSet16(&ThisPath, Start);

    Checked |= MaxPath = ThisPath;

    if (Start == End)
    {
        PathFound = true;
        goto Done;
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start)) AND NOT BitTest16(Checked, LEFT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_up_path(Data, LEFT(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start)) AND NOT BitTest16(Checked, TOP(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_up_path(Data, TOP(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start)) AND NOT BitTest16(Checked, RIGHT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_up_path(Data, RIGHT(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start)) AND NOT BitTest16(Checked, BOTTOM(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_up_path(Data, BOTTOM(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

Done:
    *pPath |= MaxPath;
    *pLength = MaxPathLength + 1;

    return PathFound;
}

static bool area_3x4_left_down_path(uint16_t Data, tIndex Start, tIndex End, uint16_t Checked, tSize *pLength, uint16_t *pPath)
{
    tSize PathLength, MaxPathLength = 0;
    uint16_t MaxPath, ThisPath = 0U;
    bool PathFound = false;

    BitSet16(&ThisPath, Start);

    Checked |= MaxPath = ThisPath;

    if (Start == End)
    {
        PathFound = true;
        goto Done;
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start)) AND NOT BitTest16(Checked, LEFT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_down_path(Data, LEFT(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start)) AND NOT BitTest16(Checked, BOTTOM(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_down_path(Data, BOTTOM(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start)) AND NOT BitTest16(Checked, RIGHT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_down_path(Data, RIGHT(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start)) AND NOT BitTest16(Checked, TOP(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_down_path(Data, TOP(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

Done:
    *pPath |= MaxPath;
    *pLength = MaxPathLength + 1;

    return PathFound;
}

static bool area_3x4_right_up_path(uint16_t Data, tIndex Start, tIndex End, uint16_t Checked, tSize *pLength, uint16_t *pPath)
{
    tSize PathLength, MaxPathLength = 0;
    uint16_t MaxPath, ThisPath = 0U;
    bool PathFound = false;

    BitSet16(&ThisPath, Start);

    Checked |= MaxPath = ThisPath;

    if (Start == End)
    {
        PathFound = true;
        goto Done;
    }

    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start)) AND NOT BitTest16(Checked, RIGHT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_down_path(Data, RIGHT(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start)) AND NOT BitTest16(Checked, TOP(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_down_path(Data, TOP(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start)) AND NOT BitTest16(Checked, LEFT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_down_path(Data, LEFT(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start)) AND NOT BitTest16(Checked, BOTTOM(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_down_path(Data, BOTTOM(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

Done:
    *pPath |= MaxPath;
    *pLength = MaxPathLength + 1;

    return PathFound;
}

static bool area_3x4_right_down_path(uint16_t Data, tIndex Start, tIndex End, uint16_t Checked, tSize *pLength, uint16_t *pPath)
{
    tSize PathLength, MaxPathLength = 0;
    uint16_t MaxPath, ThisPath = 0U;
    bool PathFound = false;

    BitSet16(&ThisPath, Start);

    Checked |= MaxPath = ThisPath;

    if (Start == End)
    {
        PathFound = true;
        goto Done;
    }

    if (RIGHT_VALID(Start) AND BitTest16(Data, RIGHT(Start)) AND NOT BitTest16(Checked, RIGHT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_down_path(Data, RIGHT(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

    if (BOTTOM_VALID(Start) AND BitTest16(Data, BOTTOM(Start)) AND NOT BitTest16(Checked, BOTTOM(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_down_path(Data, BOTTOM(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

    if (LEFT_VALID(Start) AND BitTest16(Data, LEFT(Start)) AND NOT BitTest16(Checked, LEFT(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_down_path(Data, LEFT(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

    if (TOP_VALID(Start) AND BitTest16(Data, TOP(Start)) AND NOT BitTest16(Checked, TOP(Start)))
    {
        uint16_t Path = ThisPath;
        if (area_3x4_left_down_path(Data, TOP(Start), End, Checked, &PathLength, &Path))
        {
            PathFound = true;
            SET_IF_GREATER_W_EXTRA(PathLength, MaxPathLength, Path, MaxPath);
            goto Done;
        }
    }

Done:
    *pPath |= MaxPath;
    *pLength = MaxPathLength + 1;

    return PathFound;
}

bool BitEmpty16(uint16_t Bits)
{
    return Bits == 0U;
}

bool BitTest16(uint16_t Bits, tIndex Index)
{
    return NOT BitEmpty16(Bits & (1U << Index));
}

void BitSet16(uint16_t *pBits, tIndex Index)
{
    *pBits |= (1U << Index);
}

void BitReset16(uint16_t *pBits, tIndex Index)
{
    *pBits &= ~(1U << Index);
}
