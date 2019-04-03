#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include "position.h"
#include "move.h"
#include "utils.h"
#include "eval.h"
#include "search.h"
#include "engine.h"
#include "transposition.h"

#define X_CMD_BUFSIZE   128

int nopost = 0;
engine_side_t last_side = black;


int valid_move_format(const char* str);

int xboard()
{
    setbuf(stdout, NULL);
    reset();
    engine_mode = xboard_;
    engine_side = black;
    stop_time = INF_TIME;
    stop_depth = MAX_DEPTH;

    putchar('\n');

    char cmd[X_CMD_BUFSIZE];
    char* pcmd;

    int loop = 1;
    do
    {
        fgets(cmd, X_CMD_BUFSIZE, stdin);
        pcmd = cmd;
        skip_spaces(&pcmd);

        if (strncmp(pcmd, "quit", 4) == 0)
        {
            tt_destroy();
            return 0;
        }
        else if (strncmp(pcmd, "new", 3) == 0)
        {
            reset();
            last_side = black;
            engine_side = black;
            stop_depth = MAX_DEPTH;
            stop_time = INF_TIME;
            white_clock.tc = unlimited;
            black_clock.tc = unlimited;
        }
        else if (strncmp(pcmd, "force", 4) == 0)
        {
            engine_side = arbiter;
        }
        else if (strncmp(pcmd, "post", 4) == 0)
        {
            nopost = 0;
        }
        else if (strncmp(pcmd, "nopost", 6) == 0)
        {
            nopost = 1;
        }
        else if (strncmp(pcmd, "st", 2) == 0)
        {
            pcmd += 2;
            skip_spaces(&pcmd);
            stop_time = read_num(&pcmd) * 1000;

            white_clock.tc = fixed_move;
            black_clock.tc = fixed_move;
        }
        else if (strncmp(pcmd, "sd", 2) == 0)
        {
            pcmd += 2;
            skip_spaces(&pcmd);
            stop_depth = read_num(&pcmd);
        }
        else if (strncmp(pcmd, "time", 4) == 0)
        {
            pcmd += 4;
            skip_spaces(&pcmd);
            unsigned time = read_num(&pcmd) * 10;
            if (engine_side == white)
            {
                white_clock.time = time;
            }
            else if (engine_side == black)
            {
                black_clock.time = time;
            }
            else    // force mode
            {
                if (last_side == white)
                    white_clock.time = time;
                else
                    black_clock.time = time;
            }
        }
        else if (strncmp(pcmd, "otim", 4) == 0)
        {
            pcmd += 4;
            skip_spaces(&pcmd);
            unsigned time = read_num(&pcmd) * 10;
            if (engine_side == white)
            {
                black_clock.time = time;
            }
            else if (engine_side == black)
            {
                white_clock.time = time;
            }
            else    // force mode
            {
                if (last_side == white)
                    black_clock.time = time;
                else
                    white_clock.time = time;
            }
        }
        else if (strncmp(pcmd, "level", 5) == 0)
        {
            int nmoves, time, inc;
            pcmd += 5;
            skip_spaces(&pcmd);
            nmoves = read_num(&pcmd);
            skip_spaces(&pcmd);
            time = read_num(&pcmd) * 60000;
            if (*pcmd == ':')
            {
                ++pcmd;
                time += read_num(&pcmd) * 1000;
            }
            skip_spaces(&pcmd);
            inc = read_num(&pcmd) * 1000;

            if (nmoves)
            {
                white_clock.tc = classical;
                white_clock.moves = nmoves;
                white_clock.time = time;
                white_clock.init.moves = nmoves;
                white_clock.init.time = time;

                black_clock.tc = classical;
                black_clock.moves = nmoves;
                black_clock.time = time;
                black_clock.init.moves = nmoves;
                black_clock.init.time = time;
            }
            else if (inc)
            {
                white_clock.tc = fischer;
                white_clock.time = time;
                white_clock.increment = inc;

                black_clock.tc = fischer;
                black_clock.time = time;
                black_clock.increment = inc;
            }
            else
            {
                white_clock.tc = fixed_game;
                white_clock.time = time;

                black_clock.tc = fixed_game;
                black_clock.time = time;
            }
        }
        else if (*pcmd == '?')
        {
        }
        else if (strncmp(pcmd, "go", 2) == 0)
        {
XBOARD_GO:
            if (pos.side_to_move == WHITE)
            {
                last_side = white;
                engine_side = white;
            }
            else
            {
                last_side = black;
                engine_side = black;
            }

            search();
            if (search_info.best_move.b.from != NOSQUARE)
            {
                printf("move %s\n", move_to_str(search_info.best_move));
                make_move(search_info.best_move);
            }
        }
        else
        {
            if (valid_move_format(pcmd))
            {
                move_t mv = str_to_move(pcmd);
                movelist_t* mvl = gen_moves();

                int found = -1;
                for (unsigned i = 0; i < mvl->num; i++)
                {
                    if (mvl->moves[i].u == mv.u)
                    {
                        found = i;
                        break;
                    }
                }

                if (found >= 0)
                {
                    make_move(mv);
                    if (illegal())
                    {
                        unmake_move();
                        printf("Illegal move: %s\n", pcmd);
                    }
                    else if (engine_side != arbiter)
                        goto XBOARD_GO;
                }
                else
                {
                    printf("Illegal move: %s\n", pcmd);
                }
            }
            else
                printf("Error (unknown command): %s\n", pcmd);
        }

    } while (loop);

    return 0;
}

int valid_move_format(const char* str)
{
    if (str[0] < 'a' || str[0] > 'h')
        return 0;
    if (str[1] < '1' || str[1] > '8')
        return 0;
    if (str[2] < 'a' || str[2] > 'h')
        return 0;
    if (str[3] < '1' || str[3] > '8')
        return 0;

    if (!str[4] || isspace(str[4]))
        return 1;

    if (str[4] != 'n' && str[4] != 'b' && str[4] != 'r' && str[4] != 'q')
        return 0;

    return 1;
}
