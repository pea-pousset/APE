#ifndef ENGINE_H
#define ENGINE_H

#include "clock.h"

#define VERSION_MAJOR   0
#define VERSION_MINOR   3

#define MAX_DEPTH   7
#define INF_TIME    1000000

typedef enum
{
    console,
    xboard_
} engine_mode_t;

typedef enum
{
    white, black, arbiter
} engine_side_t;

extern int nopost;  // xboard.c, turn off thinking output
extern int engine_quiet;   // CLI mode, turn off thinking output and board display

extern chessclock_t  white_clock;
extern chessclock_t  black_clock;
extern engine_side_t engine_side;
extern engine_mode_t engine_mode;
extern int           engine_is_thinking;
extern unsigned      stop_time;
extern unsigned      stop_depth;

#endif
