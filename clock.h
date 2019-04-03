#ifndef CLOCK_H
#define CLOCK_H

#define NO_TIME_LIMIT   -1

typedef enum
{
    unlimited,  ///< no time limit, search down to the max depth
    classical,  ///< a certain amount of moves must be played within a fixed time
    fixed_game, ///< a fixed time for the entire game
    fixed_move, ///< a fixed time per move
    fischer,    ///< a fixed time for the whole game + increment per move
} time_control_t;


/// Initial settings for the classical time control
typedef struct
{
    int moves;
    int time;
} classical_t;

typedef struct
{
    time_control_t tc;
    classical_t    init;
    int increment;      ///< value of the increment in fischer time
    int time;           ///< the time left
    int moves;          ///< number of moves remaining to accomplish in classical time
} chessclock_t;

void start_clock();
void stop_clock();

#endif
