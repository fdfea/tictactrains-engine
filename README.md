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

It is not very difficult to compile the TicTacTrains Engine directly from the source files. The command I most commonly use (on Windows with GCC from the `src\` directory) is:  
`gcc -Wall -O3 -lm -o tictactrains *.c`

After the program is compiled successfully, it can be run (on Windows) as:  
`.\tictactrains.exe`

##### **Compiling with CMake**

You can also compile the TicTactrains Engine with CMake. An example command (on Windows with MinGW-w64 from the root directory of the project) is:  
`cmake -S .\ -B .\build\ -G "MinGW Makefiles"`  
`cd .\build\`  
`mingw32-make`

##### **Extra Compile-time Definitions**

I recommend that you always compile the program with maximum performance optimization, namely the `-O3` flag, as it significantly speeds up the AI. There are a few other options that can be specified when compiling to add some advanced features or to print extra information during the game. To add them with GCC, use the `-D<DEF>` compiler flag. For example, `gcc ... -DDEBUG ... -o tictactrains *.c`. To add them with CMake, edit `CMakeLists.txt`, uncomment the `add_definitions()` line, and add the desired options. For example, `add_definitions(-DDEBUG ...)`. 

The options available are:
* `DEBUG` – Print any relevant debug information (will make the program slower)
* `STATS` – Print statistics for the computer opponent search tree on each move
* `TIMED` – Print the time the computer spent simulating on each move
* `PACKED` – Pack the structs in the search tree to reduce memory usage (may be slower on some architectures)
* `VISITS32` – Use a 32-bit integer for node visits in the search tree to allow for deeper searches (default is 16-bit)

### **Usage**

After starting the program, you will be asked to input your move according to the rule set. A valid move is two characters without a space corresponding to a valid square on the board, e.g. `d4`. Press enter to submit your move. If an entered move is invalid, you will be prompted until the move is valid. When playing against the computer, it will automatically make its move after you have submitted yours. When playing without the computer, the prompt will indicate which player should enter their move. After the game is finished, the score will be displayed and the program will exit. 

##### **Configuration**

The program reads from a configuration file, `ttt.conf`, to configure the game at runtime. 

The options are: 
* `COMPUTER_PLAYING` – Whether or not to play against the computer
* `COMPUTER_PLAYER` – Whether the computer will be X or O
* `RULES_TYPE` – The type of rules to play with
* `SIMULATIONS` – How many times the computer should simulate before moving
* `SCORING_ALGORITHM` – The algorithm to score simulations when not predicting
* `PREDICTION_POLICY` – When the computer should predict the score of simulations
* `PREDICTION_STRATEGY` – How the computer should predict the outcome of a simulation
* `STARTING_POSITION` – The position to start the game from

See the configuration file for additional details. 

### **Game Mechanics**

TicTacTrains is an abstract strategy game that has been likened to a combination of the popular pencil-and-paper games Tic-Tac-Toe and Dots and Boxes. 

Two players take turns placing pieces on the 7x7 board until all the squares are filled. The first player's pieces are X's and the second player's pieces are O's. The goal is to create a longer train of pieces than your opponent, connected horizontally and vertically, but not diagonally. If both players' longest train is the same length, the game ends in a draw. Once a player places a piece on a square, that piece is locked for the remainder of the game, and a piece cannot be placed on a square that is already occupied. An example of a finished board is depicted below. 

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

As you can see, X and O alternated moving once on each turn. However, the game also allows for the rules to be modified, by way of the order of the players' turns and the squares available to move on. For example, X might go first wherever they want and then O might go twice, but on their second move only be allowed to go in the outer ring of the board, depicted below. 

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

A few rule sets are provided. The default is "Classical" rules, which is notated as "(XA, OA, ...)". This means that each player alternates on every turn and can go in any square, hence the "A". One of the other rule sets provided is "Modern" rules, which is notated as "(XA, O1, O3, XA, OA, ...)". This means that X goes once anywhere, O goes twice (once in ring 1 and once in ring 3), and then the players alternate for the rest of the game, going wherever is available. Additional rules can be created by modifying the code. 

### **Explanation of AI**

The artificially intelligent opponent uses the Monte Carlo Tree Search algorithm. Essentially, it works by expanding a search tree from the current game state. The AI strategically works its way down the tree until it finds a leaf node and then expands the node's children. Then it simulates a playout from one of the new nodes and propagates the result back up to the root of the tree. It repeats this process for a given number of simulations. The tree looks like a minimax tree, but the nodes with the best score are explored more, and the root child with the most visits is ultimately the one that the AI chooses. This allows the AI to avoid exploring nodes that are statistically unlikely to be good, saving a lot of time compared to minimax. You will find that the AI is very strong with 10000 or more simulations per move, taking an average of about 400 ms on my machine, when optimal scoring and sufficient compiler optimization are used. 

The search tree can also get rather large when a high number of simulations are used. This is one of the main reasons I used C to implement the game, as I was able to condense each search tree node into a minimum of 31 bytes, which makes the size of the tree negligible for just about any device or use case. 

However, when the paths get long during the game (longer than about 12 pieces), the engine can get rather slow because the simulation scoring algorithm is technically NP-hard (longest path problem). To combat this, I implemented a few machine learning models to predict the outcome of a simulation, namely linear regression, logistic regression, and a neural network regressor. The linear and logistic regression models predict the score of a board using the number of pieces in a given board area and the neighbor count of each square (0-4). For example, a given board area might look something like:

```
7 [ ][ ][ ][ ][ ][ ][ ]
6 [ ][ ][ ][ ][ ][ ][ ]
5 [ ][ ][X][ ][X][X][ ]
4 [ ][X][X][X][X][X][ ]
3 [ ][ ][ ][X][ ][ ][ ]
2 [ ][ ][ ][ ][ ][ ][ ]
1 [ ][ ][ ][ ][ ][ ][ ]
&  a  b  c  d  e  f  g
```

There are 9 pieces in this board area, and each piece is associated with its neighbors by counting the number of pieces that are directly adjacent (left, right, top, or bottom). The linear regression model finds all the board areas for each player, predicts the longest path through each board area, and determines a winner by deciding which player had the longest predicted path. The logistic regression model finds the best board area for each player using linear regression, uses a logistic model to compare the two best areas, and then returns its evaluation of which player is most likely to be the winner. With these regression models I achieved an accuracy of about 87-93% (in terms of mean absolute error in path length prediction and predicted win versus true win, respectively).

The multilayer perceptron neural network regressor predicts the outcome of a game by looking at the shape of all the board areas for each player. The shape is determined by looking at each square in the area and the 8 surrounding squares. The model classifies each square in a board area into one of 33 shapes and does this for each square in each area. Then the data is fed into the neural network, which outputs the predicted path length of each area. The longest predicted paths for each player are compared to determine the winner. I achieved about 95% accuracy (in terms of mean absolute error in predicted path length) with this method. 

The models were each trained with millions of randomly simulated games. They make the AI faster and no longer scale with path length, but are less accurate when a high degree of precision is needed (in the endgame) as a result of the models' criteria for a win being slightly different than the true criteria due to information loss during feature extraction. I will also note that it not expected that these models would be extremely accurate because the problem for scoring the game is NP-hard, so you wouldn't expect that there would be a way to do it with extreme accuracy in much less time. 

### **Credits**

Developer and Creator - Forrest Feaser ([@fdfea](https://github.com/fdfea))
