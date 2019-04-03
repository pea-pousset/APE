#ifndef MOVE_H
#define MOVE_H

#define UP          16
#define DOWN        -16
#define LEFT        -1
#define RIGHT       1
#define UPLEFT      (UP + LEFT)
#define UPRIGHT     (UP + RIGHT)
#define DOWNLEFT    (DOWN + LEFT)
#define DOWNRIGHT   (DOWN + RIGHT)
#define UPLEFT2     (UP + LEFT + LEFT)
#define UP2LEFT     (UP + UP + LEFT)
#define UP2RIGHT    (UP + UP + RIGHT)
#define UPRIGHT2    (UP + RIGHT + RIGHT)
#define DOWNLEFT2   (DOWN + LEFT + LEFT)
#define DOWN2LEFT   (DOWN + DOWN + LEFT)
#define DOWN2RIGHT  (DOWN + DOWN + RIGHT)
#define DOWNRIGHT2  (DOWN + RIGHT + RIGHT)

typedef union
{
    struct
    {
        char from;
        char to;
        char promotion;
        char bits;  // unused
    } b;
    int u;
} move_t;

typedef struct
{
    int num;
    move_t moves[256];
    int score[256];
} movelist_t;

extern const int deltas[7][8];

movelist_t* gen_moves();
movelist_t* gen_captures();
int         attacked(char sq, char by);

#endif
