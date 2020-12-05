#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "position.h"
#include "move.h"
#include "utils.h"
#include "eval.h"
#include "search.h"
#include "engine.h"
#include "transposition.h"

#define CMD_BUFSIZE 128

char cmd[CMD_BUFSIZE];

extern int   xboard();
unsigned int perft(unsigned int depth);
unsigned int divide(unsigned int depth);
void         bench();
void         help();

int main(int argc, char** argv)
{
    printf("APE v%d.%d\n", VERSION_MAJOR, VERSION_MINOR);
    printf("Press 'enter' to display the board. Type \"help\" to see a list of commands.\n\n");
    fflush(stdout);

    tt_init();
    reset();
    initial_hash = pos.hash;    // save the initial position hash

    engine_quiet = 0;
    engine_mode = console;
    engine_side = black;
    engine_is_thinking = 0;
    stop_time = INF_TIME;
    stop_depth = INIT_DEPTH;
    white_clock.tc = unlimited;
    black_clock.tc = unlimited;


    // set_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"); // position 2 "kiwipete", ok (D5)
    // set_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");    // position 3, ok (D7)
    // set_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"); // position 4, ok (D6)
    // set_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"); // position 5, ok (D5)

    // set_fen("5Kbk/6pp/6P1/8/8/8/8/7R w - - 0 1"); // mate in 2
    // set_fen("r5rk/5p1p/5R2/4B3/8/8/7P/7K w - - 0 1"); // mate in 3
    // set_fen("2k3r1/pp6/8/2pPnb2/2P2q2/1P3P2/P5RN/3R2K1 b - - 0 1"); // mate in 4

    // set_fen("1nb1kbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQ - 0 1");

    // set_fen("4k3/1P6/8/8/8/8/8/2K5 w KQkq - 0 1");
    // set_fen("3k4/P5p1/3pPp1P/4pP2/8/8/1PpP3p/2K5 w - - 0 1");

    // set_fen("3k4/1P4p1/2P3P1/8/8/4p1p1/2p1p1Pp/4K3 w - - 0 1 "); // GOOD PAWN STRUCT TEST
    // set_fen("8/p4ppk/1p6/2p1r3/2P2n2/P4P1p/5P1P/2q2K2 w - - 0 50 ");


    cmd[0] = 0;
    int loop = 1;
    char* pcmd;
    do
    {
        printf("> ");
        fgets(cmd, CMD_BUFSIZE, stdin);

        pcmd = cmd;
        skip_spaces(&pcmd);

        if (strncmp(pcmd, "xboard", 6) == 0)
        {
            return xboard();
        }
        else if (strncmp(pcmd, "quit", 4) == 0)
        {
            loop = 0;
        }
        else if (strncmp(pcmd, "help", 4) == 0)
        {
            help();
        }
        else if (strncmp(pcmd, "new", 3) == 0)
        {
            reset();
            print();
            engine_side = black;
        }
        else if (strncmp(pcmd, "undo", 4) == 0)
        {
            unmake_move();
            print();
        }
        else if (strncmp(pcmd, "arbiter", 7) == 0)
        {
            engine_side = arbiter;
            printf("Arbiter mode.\n\n");
        }
        else if (*pcmd == '\0' || strncmp(pcmd, "print", 5) == 0)
        {
            print();
        }
        else if (strncmp(pcmd, "fen", 3) == 0)
        {
            printf("%s\n\n", get_fen());
        }
        else if (strncmp(pcmd, "time", 4) == 0)
        {
            pcmd += 4;
            skip_spaces(&pcmd);
            stop_time = read_num(&pcmd) * 1000;
            if (!stop_time)
            {
                white_clock.tc = unlimited;
                black_clock.tc = unlimited;
                printf("Infinite earch time.\n\n");
            }
            else
            {
                white_clock.tc = fixed_move;
                black_clock.tc = fixed_move;
                printf("Search time set to %d seconds.\n\n", stop_time / 1000);
            }

        }
        else if (strncmp(pcmd, "depth", 5) == 0)
        {
            pcmd += 5;
            skip_spaces(&pcmd);
            stop_depth = read_num(&pcmd);
            if (!stop_depth || stop_depth > MAX_DEPTH)
                stop_depth = MAX_DEPTH;

            printf("Search depth set to %d plies.\n\n", stop_depth);
        }
        else if (strncmp(pcmd, "perft", 5) == 0)
        {
            pcmd += 5;
            skip_spaces(&pcmd);
            unsigned int depth = read_num(&pcmd);
            if (depth > 0)
            {
                if (depth > MAX_DEPTH)
                    depth = MAX_DEPTH;
                printf("running perft to depth %d...\n", depth);
                int tstart = get_time_ms();
                unsigned int res = perft(depth);
                int tend = get_time_ms();
                float time = (float)(tend - tstart) / 1000.f;
                printf("%d nodes explored in %.3fs (%.1f mnps)\n\n", res, time, res / (time * 1000000.f));
            }
            else
                printf("error.\n\n");
        }
        else if (strncmp(pcmd, "divide", 6) == 0)
        {
            pcmd += 6;

            while (*pcmd && isspace(*pcmd))
                pcmd++;

            unsigned int depth = read_num(&pcmd);
            if (depth > 0)
            {
                if (depth > MAX_DEPTH)
                    depth = MAX_DEPTH;
                printf("running divide to depth %d...\n", depth);
                divide(depth);
            }
            else
                printf("usage: divide <depth>.\n\n");
        }
        else if (strncmp(pcmd, "eval", 4) == 0)
        {
            printf("%.2f\n", eval() / 100.f);
            // eval(); // #################################################################### DEBUG
        }
        else if (strncmp(pcmd, "bench", 5) == 0)
        {
            bench();
        }
        else if (strncmp(pcmd, "quiet", 5) == 0)
        {
            pcmd += 5;

            while (*pcmd && isspace(*pcmd))
                pcmd++;

            if (strncmp(pcmd, "off", 3) == 0)
            {
                engine_quiet = 0;
            }
            else if (strncmp(pcmd, "on", 2) == 0)
            {
                engine_quiet = 1;
            }
            else
            {
                printf("Invalid command.\n");
            }

        }
        else if (strncmp(pcmd, "go", 2) == 0)
        {
GO:
            engine_is_thinking = 1;

            if (pos.side_to_move == WHITE)
                engine_side = white;
            else
                engine_side = black;

            search();
            if (search_info.best_move.b.from != NOSQUARE)
            {
                make_move(search_info.best_move);
                print();
            }
            engine_is_thinking = 0;
        }
        else
        {
            move_t mv = str_to_move(pcmd);
            if (mv.b.from != NOSQUARE)
            {
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
                        printf("illegal move.\n\n");
                    }
                    else
                    {
                        print();
                        if (engine_side != arbiter)
                            goto GO;
                    }
                }
                else
                {
                    printf("illegal move.\n\n");
                }
            }
            else
                printf("illegal move.\n\n");
        }

    } while(loop);

    tt_destroy();
    return 0;
}




unsigned int perft(unsigned int depth)
{
    if (depth == 0)
        return 1;

    unsigned int count = 0;

    movelist_t* mvl = gen_moves();
    for (unsigned int i = 0; i < mvl->num; i++)
    {
        make_move(mvl->moves[i]);
        ++ply;

        if (!illegal())
            count += perft(depth - 1);
        unmake_move();
        --ply;
    }

    return count;
}

unsigned int divide(unsigned int depth)
{
    if (depth == 0)
        return 1;

    unsigned int cmoves = 0;
    unsigned int cnodes = 0;

    movelist_t* mvl = gen_moves();
    for (unsigned int i = 0; i < mvl->num; i++)
    {
        unsigned int cperft = 0;
        char c = "  NBRQK"[TYPE(pos.board[mvl->moves[i].b.from])];
        char d = pos.board[mvl->moves[i].b.to] ? 'x' : '-';

        make_move(mvl->moves[i]);
        ++ply;

        if (!illegal())
        {
            cperft = perft(depth - 1);
            cmoves++;
            cnodes += cperft;
            putchar(c);
            printf("%c%d", "abcdefgh"[FILE(mvl->moves[i].b.from)], RANK(mvl->moves[i].b.from) + 1);
            putchar(d);
            printf("%c%d", "abcdefgh"[FILE(mvl->moves[i].b.to)], RANK(mvl->moves[i].b.to) + 1);
            printf("%c%c ", " ======"[mvl->moves[i].b.promotion], " PNBRQK"[mvl->moves[i].b.promotion]);
            printf("%d \n", cperft);
        }
        unmake_move();
        --ply;
    }
    printf("%d moves\n%d nodes\n\n", cmoves, cnodes);

    return cnodes;
}

void bench()
{
    set_fen("1rb2rk1/p4ppp/1p1qp1n1/3n2N1/2pP4/2P3P1/PPQ2PBP/R1B1R1K b - - 0 17");
    print();
    unsigned int tstart, tend;
    int st = stop_time;
    int sd = stop_depth;
    stop_time = INF_TIME;
    stop_depth = 6;
    int best_time = INF_TIME * 1000;
    int worst_time = 0;
    for (int i = 0; i < 5; i++)
    {
        tstart = get_time_ms();
        search();
        tend = get_time_ms();
        int time = tend - tstart;
        if (time < best_time)
            best_time = time;
        if (time > worst_time)
            worst_time = time;
    }
    printf("Best score: %.2fs - %.2f millions nodes per second\n",
           best_time / 1000.f,
           (1000.f * search_info.nodes) / (1000000.f * (float)(best_time)));
    printf("Worst:      %.2fs - %.2f \"\n",
           worst_time / 1000.f,
           (1000.f * search_info.nodes) / (1000000.f * (float)(worst_time)));
    stop_time = st;
    stop_depth = sd;
}

void help()
{
    printf("new:            Start a new game as white, leaving the arbiter mode.\n");
    printf("go:             Force the engine to play the current side to move, leaving the arbiter mode.\n");
    printf("undo:      \n");
    printf("print:          Display the board. Alternatively you can just press 'enter'\n");
    printf("                without entering a command.\n");
    printf("depth <depth>:  Sets the max depth at which the engine will search moves (1-%d).\n", MAX_DEPTH);
    printf("time [seconds]: Sets the max search time - no value means infinite time until max depth is reached.\n");
    printf("arbiter:        Activate the arbiter mode.\n");
    printf("perft <depth>:  Run a performance test down to the chosen depth (1-%d).\n", MAX_DEPTH);
    printf("divide <depth>: Run a detailed performance test down to the chosen depth (1-%d).\n", MAX_DEPTH);
    printf("bench:          Run a performance test evaluating a position\n");
    printf("help:           Display this message.\n");
    printf("quit:           :(\n\n");
}
