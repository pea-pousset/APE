#include "clock.h"
#include "engine.h"
#include "utils.h"
#include "position.h"
#include "search.h"

/*========================================================================*//**
 * \brief Start the engine's own clock, allocating time for the search.
 *//*=========================================================================*/
void start_clock()
{
    search_info.start_time = get_time_ms();
    chessclock_t* c = (pos.side_to_move == white ? &white_clock : &black_clock);
    if (c->tc == unlimited)
    {
        search_info.alloc_time = NO_TIME_LIMIT;
    }
    else if (c->tc == fixed_move)
    {
        search_info.alloc_time = stop_time;
    }
    else if (c->tc == classical)
    {
        search_info.alloc_time = c->time / c->moves;
    }
    else if (c->tc == fixed_game)
    {
        // Waste time in the opening to ensure we cannot find a mate in 3 in the endgame
        search_info.alloc_time = c->time / 35;
    }
    else if (c->tc == fischer)
    {
        int ttime = c->time - c->increment;
        if (ttime < 0)
            ttime = 0;
        search_info.alloc_time = (ttime / 35) + c->increment;
    }
}

/*========================================================================*//**
 * \brief Stop the engine's own clock and update it. The host (or gui)
 * should send time & otim after every move anyway.
 *//*=========================================================================*/
void stop_clock()
{
    unsigned time = get_time_ms() - search_info.start_time;
    chessclock_t* c = pos.side_to_move == white ? &white_clock : &black_clock;

    if (c->tc == unlimited || c->tc == fixed_move)
    {
        // You do nothing John Doe
    }
    else if (c->tc == classical)
    {
        c->time -= time;
        c->moves--;
        if (!c->moves)
        {
            c->time += c->init.time;
            c->moves = c->init.moves;
        }
    }
    else if (c->tc == fixed_game)
    {
        c->time -= time;
    }
    else if (c->tc == fischer)
    {
        c->time -= time;
        c->time += c->increment;
    }
}
