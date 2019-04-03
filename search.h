#ifndef SEARCH_H
#define SEARCH_H

#include "move.h"

#define MATE    -10000

typedef struct
{
    move_t      best_move;  ///< Current best move
    int         score;      ///< Score of the current best move
    unsigned    iroot_move; ///< Index of the root move currently searched
    unsigned    depth;      ///< Current search depth
    unsigned    start_time; ///< Start time of search
    unsigned    alloc_time; ///< Allocated time for the search
    unsigned    nodes;      ///< Number of explored nodes
} search_info_t;

extern search_info_t search_info;

extern int heuristic_his[128][128];
extern int killers[2][128][128];

/// \brief   The current ply.
/// \warning It is used by the move generator
extern int ply;

/// Set this flag to 1 to immediately stop the search.
extern volatile int stop_search;

void search();

#endif
