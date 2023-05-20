# **TicTacTrains Engine**

### **Table of Contents**
1. [Summary](#summary)
2. [Compiling](#compiling)
3. [Usage](#usage)
4. [Game Mechanics](#game-mechanics)
5. [Explanation of AI](#explanation-of-ai)
6. [Credits](#credits)

### **Summary**

An implementation of the board game TicTacTrains in C, including an artificially intelligent computer opponent and configurable rules.

### **Compiling**

##### **Compiling from Source**

To compile TicTacTrains Engine directly from the source files, the command I most commonly use (on Windows with GCC from the `src\` directory) is:  
`gcc -Wall -O3 -lm -o tictactrains *.c`

After the program is compiled successfully, it can be run (on Windows) as:  
`.\tictactrains.exe`

##### **Compiling with CMake**

You can also compile TicTactrains Engine with CMake. An example command (on Windows with MinGW-w64 from the root directory of the project) is:  
`cmake -S .\ -B .\build\ -G "MinGW Makefiles"`  
`cd .\build\`  
`mingw32-make`

##### **Extra Compile-time Definitions**

I recommend that you always compile the program with maximum performance optimization (the `-O3` flag on GCC), as it significantly speeds up the AI. There are a few other options that can be specified when compiling to add some advanced features or to print extra information during the game. To add them with GCC, use the `-D<DEF>` compiler flag. For example, `gcc ... -DDEBUG ... -o tictactrains *.c`. To add them with CMake, edit `CMakeLists.txt`, uncomment the `add_definitions()` line, and add the desired options. For example, `add_definitions(-DDEBUG ...)`. 

The definitions available are:
* `DEBUG` – Print debug information (may make the program slower)
* `STATS` – Print statistics for the computer opponent search tree on each move
* `TIMED` – Print the time the computer spent simulating on each move
* `SPEED` – Optimize computer opponent for speed (25-100% faster with some extra memory overhead)
* `PACKED` – Pack the structs in the search tree to reduce memory usage (may be slower on some architectures)
* `VISITS32` – Use a 32-bit integer for node visits in the search tree to allow for deeper searches (default is 16-bit)

### **Usage**

After starting the program, you will be asked to input your move according to the rule set. A valid move is two characters without any spaces corresponding to a valid square on the board, e.g. `d4`. Press enter to submit your move. If you enter an invalid move, you will be prompted until your input is valid. When playing against the computer, it will automatically make its move after you have submitted yours. When playing without the computer, the prompt will indicate which player should enter their move. After the game is finished, the score will be displayed and the program will exit. 

##### **Configuration**

The program reads from a configuration file, `ttt.conf`, to configure the game at runtime. 

The options are: 
* `COMPUTER_PLAYING` – Whether to play against the computer
* `COMPUTER_PLAYER` – If the computer is playing, whether the computer or player should move first
* `RULES_TYPE` – The rule set to use
* `SIMULATIONS` – How many simulations the computer should run before making a move
* `SEARCH_ONLY_NEIGHBORS` – Whether the computer should only search neighboring states (i.e. states in which the next move is a square that is directly adjacent or diagonal to an occupied square)
* `STARTING_POSITION` – A list of moves from which to start the game

See the configuration file for additional details. 

### **Game Mechanics**

TicTacTrains is an abstract strategy game that has been likened to a combination of the popular pencil-and-paper games Tic-Tac-Toe and Dots and Boxes. 

Two players take turns placing pieces on the 7x7 board until all the squares are occupied. The first player's pieces are X's and the second player's pieces are O's. The goal is to create the longest train, i.e. sequence, of pieces, connected horizontally and vertically, but not diagonally. The player with the longest train wins. If both players' longest train is the same length, the game ends in a draw. Once a player places a piece on a square, that piece is locked for the remainder of the game, and a piece cannot be placed on a square that is already occupied. An example of a finished board is depicted below. 

```
7 [X][X][O][X][O][O][O]
6 [X][X][O][O][X][X][X]
5 [O][O][O][O][O][O][X]
4 [X][O][O][X][X][X][O]
3 [X][X][X][X][X][O][O]
2 [O][X][O][X][O][X][O]
1 [X][X][X][O][O][X][O]
&  a  b  c  d  e  f  g
```

In this game, O won because they achieved a path length of 10, while X achieved a path length of only 9. The order of the moves in this game went as follows: 

```
 1. d4 d5   2. e4 c4   3. d3 e5
 4. f4 c5   5. c3 f5   6. b3 b4
 7. g5 g4   8. a4 f3   9. f6 b5
10. b6 a5  11. a6 e2  12. b2 d6
13. e6 c2  14. a3 a2  15. b1 c6
16. c1 d1  17. d7 c7  18. b7 e1
19. d2 g3  20. e3 e7  21. a1 g7
22. f2 g2  23. f1 g1  24. a7 f7
25. g6
```

As you can see, X and O alternated moving once on each turn. However, the game also allows for the rules to be configured, by way of the order of the players' turns and the squares available to move on. For example, X might go first wherever they want and then O might go twice, but on their second move only be allowed to go in the outer ring of the board, depicted below. 

```
1. d4 d5 g4   2. c4 e4
7 [ ][ ][ ][ ][ ][ ][ ]
6 [ ][ ][ ][ ][ ][ ][ ]
5 [ ][ ][ ][O][ ][ ][ ]
4 [ ][ ][X][X][O][ ][O]
3 [ ][ ][ ][ ][ ][ ][ ]
2 [ ][ ][ ][ ][ ][ ][ ]
1 [ ][ ][ ][ ][ ][ ][ ]
&  a  b  c  d  e  f  g
```

When the rules only allow a player to make a move in certain squares, the board is generally split into three rings (1, 2, and 3), shown below. Usually a player's movement is restricted only in the opening of the game (the first 3 turns or so). 

```
7 [3][3][3][3][3][3][3]
6 [3][2][2][2][2][2][3]
5 [3][2][1][1][1][2][3]
4 [3][2][1][1][1][2][3]
3 [3][2][1][1][1][2][3]
2 [3][2][2][2][2][2][3]
1 [3][3][3][3][3][3][3]
&  a  b  c  d  e  f  g
```

These rule modifications helps to keep the gameplay fresh and can make the game more fair in some cases. A few rule sets are provided with TicTacTrains Engine. The default is "Classical" rules, which is notated as "(XA, OA, ...)". This means that each player alternates on every turn and can go in any square, hence the "A". Another rule sets provided is "Modern" rules, which is notated as "(XA, O1, O3, XA, OA, ...)". This means that X goes once anywhere, O goes twice (once in ring 1 and once in ring 3), and then the players alternate for the rest of the game, going wherever is available. Additional rule sets can be created by modifying the code. 

### **Explanation of AI**

The artificially intelligent opponent uses the Monte Carlo Tree Search (MCTS) algorithm. The algorithm works by expanding a search tree from the current game state. The AI strategically works its way down the tree until it finds a leaf node, at which point it expands the node's children. Next, it simulates a playout from one of the new child nodes and propagates the result back up to the root of the tree. It repeats this process for a given number of simulations. The tree looks like a minimax tree, but the nodes with the best score are explored more, and the root node child with the most visits is ultimately the one that the AI chooses. This allows the AI to avoid exploring nodes that are statistically unlikely to be good, saving a lot of time compared to minimax. You will find that the AI is very strong with 10000 or more simulations per move. On my machine (Intel Core i7-10750H, Windows 10, MinGW-w64 GCC 8.1.0), 10000 simulations takes about 200 ms on average when sufficient compiler optimization is used. However, if paths are long, scoring can take noticeably longer, due to the fact that finding the longest path in a graph is an NP-complete problem in the general case. 

The search tree can also get quite large when a high number of simulations are used. This is one of the main reasons I used C to implement the engine, as I was able to condense each search tree node into a minimum of 31 bytes, which makes the size of the tree negligible for just about any device or use case. 

For extreme optimization, I implemented a lookup table to precompute paths for scoring. I divided the 7x7 board into four 3x4 grids with one 1x1 grid in the center. The lookup table stores every path from every valid index to every valid exit in each 3x4 area and computes rotations so the paths can be shared between the quadrants of the board. Fortunately, not every one of these paths needs to be search, I implemented some heuristics to reduce the total paths in the lookup table from about 116000 to about 80000, but many of them are specific to the dimensions of the area. Using the lookup table, the scorer starts at an index of the board and iterates over all the paths to each exit, and, if the exit connects to a path in another quadrant, it continues to search until it finds the longest path. Essentially, the lookup table reduces the search space by trading depth of search for breadth of search. I've found that in the worst case you can expect about a 25% increase in performance, but in practice it is often twice as fast as the brute-force algorithm. 

### **Credits**

Developer and Creator - Forrest Feaser ([@fdfea](https://github.com/fdfea))
