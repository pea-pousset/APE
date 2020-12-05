#include "search.h"
#include "position.h"
#include "utils.h"
#include "engine.h"
#include "eval.h"
#include "clock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <setjmp.h>

search_info_t search_info;
int heuristic_his[128][128];
int killers[2][128][128];
int ply = 0;
volatile int stop_search;

static jmp_buf env;
static int nodes;
static int follow_pv;
static move_t pv[MAX_DEPTH][MAX_DEPTH];
static int pv_len[MAX_DEPTH + 1];

static void print_pvline();
static void root(int depth);
static int  pvs(int alpha, int beta, int depth);
static int  quiesce(int alpha, int beta);
static void search_pv(movelist_t* mvl);
static void next_best(movelist_t* mvl, int from);
static void check_stop();

void search()
{
    start_clock();

    memset(pv, 0, sizeof(pv));
    memset(heuristic_his, 0, sizeof(heuristic_his));
    memset(killers, 0, sizeof(killers));
    ply = 0;
    stop_search = 0;

    search_info.best_move.b.from = NOSQUARE;
    search_info.score = 0;
    search_info.iroot_move = 0;
    search_info.depth = 1;
    search_info.nodes = 0;

    setjmp(env);
    if (stop_search)
    {
        while (ply)
        {
            unmake_move();
            --ply;
        }
        stop_clock();
        return ;
    }

    if ((engine_mode != xboard_) && (!engine_quiet))
    {
        printf("Ply  Nodes      Score   PV\n");
        printf("---  -----      -----   --\n");
    }

    for ( ; search_info.depth <= stop_depth; search_info.depth++)
    {
        follow_pv = 1;
        search_info.score = pvs(-10000, 10000, search_info.depth);
        search_info.best_move = pv[0][0];
        print_pvline();
        if (search_info.score > 9000 || search_info.score < -9000)
            break;
    }

    if ((engine_mode != xboard_) && (!engine_quiet)
        && (search_info.score > 9000 || search_info.score < -9000))
    {
        int in = abs(MATE) - abs(search_info.score) - 1;
        in = in / 2 + in % 2;
        if (in > 0)
            printf("Mate in %d\n\n", in);
        else
            printf("CHECKMATE\n");
    }

    ply = 0;
    putchar('\n');

    stop_clock();
}

void print_pvline()
{
    int time = (get_time_ms() - search_info.start_time);

    if (engine_mode != xboard_)
    {
        if (engine_quiet)
            return;
        
        printf("%-3d  %-9d  %-+6.2f  ",
               search_info.depth,
               search_info.nodes,
               ((search_info.score / 100.f) * (pos.side_to_move ? -1.f : 1.f))
               );
        for (int i = 0; i < pv_len[0] && i < 10; i++)
            printf("%s ", move_to_str(pv[0][i]));
        putchar('\n');
    }
    else if (!nopost)
    {
        printf("%d %d %d %d ",
               search_info.depth,
               search_info.score,
               time / 10,
               search_info.nodes
               );
        for (int i = 0; i < pv_len[0]; i++)
            printf("%s ", move_to_str(pv[0][i]));
        putchar('\n');
    }
}

void root(int depth)
{
    static movelist_t* mvl;

    mvl = gen_moves();
    for (int i = 0; i < mvl->num; i++)
    {
        search_info.iroot_move = i;
    }
}

/*========================================================================*//**
 *
 *//*=========================================================================*/
int pvs(int alpha, int beta, int depth)
{
    if (depth == 0)
        return quiesce(alpha, beta);

    ++search_info.nodes;

    if (search_info.nodes & 1023)
        check_stop();

    if (ply && is_repetition())
        return 0;

    if (pos.half_moves >= 100)
        return 0;

    if (ply >= MAX_DEPTH)
        return eval();

    int check = in_check(pos.side_to_move);
    if (check)
        ++depth;

    pv_len[ply] = ply;
    movelist_t* mvl = gen_moves();

    if (follow_pv)
        search_pv(mvl);

    int legal = 0;
    for (int i = 0; i < mvl->num; i++)
    {
        next_best(mvl, i);

        make_move(mvl->moves[i]);
        ++ply;

        if (illegal())
        {
            unmake_move();
            --ply;
            continue;
        }
        legal = 1;

        int score;
        if ((follow_pv && i == 0) || depth < 4)
            score = -pvs(-beta, -alpha, depth - 1);
        else
        {
            score = -pvs(-alpha - 1, -alpha, depth - 1);
            if (score > alpha)
                score = -pvs(-beta, -alpha, depth - 1);
        }

        unmake_move();
        --ply;

        heuristic_his[mvl->moves[i].b.from][mvl->moves[i].b.to] += depth;
        if (score > alpha)
        {
            if (score >= beta)
            {
                killers[pos.side_to_move][mvl->moves[i].b.from][mvl->moves[i].b.to] += depth * depth;
                return beta;
            }

            alpha = score;

            pv[ply][ply] = mvl->moves[i];
            for (int j = ply + 1; j < pv_len[ply + 1]; ++j)
                pv[ply][j] = pv[ply + 1][j];
            pv_len[ply] = pv_len[ply + 1];

            if (ply == 0)
                search_info.best_move = mvl->moves[i];
        }
    }

    if (!legal)
    {
        if (check)
            return MATE + ply;
        return 0;
    }

    return alpha;
}

int quiesce(int alpha, int beta)
{
    search_info.nodes++;

    if (search_info.nodes & 1023)
        check_stop();


    if (ply >= MAX_DEPTH)
        return eval();

    int score = eval();
    if (score >= beta)
        return beta;
    if (score > alpha)
        alpha = score;

    pv_len[ply] = ply;
    movelist_t* mvl = gen_captures();
    if (follow_pv)
        search_pv(mvl);


    for (int i = 0; i < mvl->num; i++)
    {
        next_best(mvl, i);
        make_move(mvl->moves[i]);
        ++ply;

        if (illegal())
        {
            unmake_move();
            --ply;
            continue;
        }

        score = -quiesce(-beta, -alpha);

        unmake_move();
        --ply;

        if (score > alpha)
        {
            if (score >= beta)
                return beta;
            alpha = score;

            pv[ply][ply] = mvl->moves[i];
            for (int j = ply + 1; j < pv_len[ply + 1]; ++j)
                pv[ply][j] = pv[ply + 1][j];
            pv_len[ply] = pv_len[ply + 1];
        }
    }

    return alpha;
}

void search_pv(movelist_t* mvl)
{
    for (int i = 0; i < mvl->num; i++)
    {
        if (mvl->moves[i].u == pv[0][ply].u)
        {
            mvl->score[i] += 10000000;
            return;
        }
    }
    follow_pv = 0;
}

void next_best(movelist_t* mvl, int from)
{
    int best_score = mvl->score[from];
    int best_index = from;
    for (int i = from + 1; i < mvl->num; i++)
    {
        if (mvl->score[i] > best_score)
        {
            best_score = mvl->score[i];
            best_index = i;
        }
    }

    if (best_index != from)
    {
        move_t tmp = mvl->moves[best_index];
        int tmps = mvl->score[best_index];
        mvl->moves[best_index] = mvl->moves[from];
        mvl->score[best_index] = mvl->score[from];
        mvl->moves[from] = tmp;
        mvl->score[from] = tmps;
    }
}

void check_stop()
{
    unsigned int time = (get_time_ms() - search_info.start_time);
    if (search_info.alloc_time != NO_TIME_LIMIT
        && time >= search_info.alloc_time)
        stop_search = 1;

    if (stop_search)
    {
        longjmp(env, 0);
    }
}
