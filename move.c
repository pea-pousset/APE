#include "move.h"
#include "position.h"
#include "eval.h"
#include "search.h"
#include "engine.h"

#include <string.h>
#include <stdlib.h>

#define SORT_CAPTURE    1000000
#define SORT_PROMOTION   900000
#define SORT_KILLER      800000

const unsigned num_deltas[7] = { 0, 0, 8, 4, 4, 8, 8 };

const unsigned num_squares[7] = { 0, 0, 1, 7, 7, 7, 1 };

const int deltas[7][8] =
{
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { UPLEFT2, UP2LEFT, UP2RIGHT, UPRIGHT2, DOWNLEFT2, DOWN2LEFT, DOWN2RIGHT, DOWNRIGHT2 },
    { UPRIGHT, UPLEFT, DOWNRIGHT, DOWNLEFT, 0, 0, 0, 0 },
    { UP, DOWN, RIGHT, LEFT, 0, 0, 0, 0 },
    { UP, DOWN, RIGHT, LEFT, UPRIGHT, UPLEFT, DOWNRIGHT, DOWNLEFT },
    { UP, DOWN, RIGHT, LEFT, UPRIGHT, UPLEFT, DOWNRIGHT, DOWNLEFT }
};

static movelist_t mvlsts[MAX_DEPTH + 1];

inline void add_quiet(movelist_t* mvl, int from, int to);
inline void add_promo(movelist_t* mvl, int from, int to);
inline void add_cap(movelist_t* mvl, int from, int to, int captype);
inline void add_promocap(movelist_t* mvl, int from, int to, int captype);

movelist_t* gen_moves()
{
    movelist_t* mvl = mvlsts + ply;
    mvl->num = 0;

    int sq;
    int type;
    int target;
    int pcap;
    for (unsigned i = 0; i < 16; i++)
    {
        sq = pos.pieces[pos.side_to_move][i];
        if (sq == NOSQUARE)
            continue;

        type = TYPE(pos.board[sq]);

        if (type == PAWN)
        {
            int dir = pos.side_to_move == WHITE ? UP : DOWN;
            target = sq + dir;

            // Pawn push
            if (!OFFBOARD(target))
            {
                if (!pos.board[target])
                {
                    if (RANK(target) != _8 && RANK(target) != _1)
                    {
                        add_quiet(mvl, sq, target);

                        // Two squares move
                        if ((RANK(sq) == _2 && pos.side_to_move == WHITE)
                            || (RANK(sq) == _7 && pos.side_to_move == BLACK))
                        {
                            if (pos.board[target + dir] == 0)
                                add_quiet(mvl, sq, target + dir);
                        }
                    }
                    else
                        add_promo(mvl, sq, target);
                }
            }

            // Pawn left side capture
            target = sq + dir + LEFT;
            if (!OFFBOARD(target))
            {
                pcap = pos.board[target];
                if ((pcap && COLOR(pcap) != pos.side_to_move)
                    || (target == pos.en_passant))
                {
                    if (RANK(target) != _8 && RANK(target) != _1)
                    {
                        if (target == pos.en_passant)
                            add_cap(mvl, sq, target, PAWN);
                        else
                            add_cap(mvl, sq, target, TYPE(pcap));
                    }
                    else
                        add_promocap(mvl, sq, target, TYPE(pcap));
                }

            } // Left side pawn capture

            // Right side pawn capture
            target = sq + dir + RIGHT;
            if (!OFFBOARD(target))
            {
                pcap = pos.board[target];
                if ((pcap && COLOR(pcap) != pos.side_to_move)
                    || (target == pos.en_passant))
                {
                    if (RANK(target) != _8 && RANK(target) != _1)
                    {
                        if (target == pos.en_passant)
                            add_cap(mvl, sq, target, PAWN);
                        else
                            add_cap(mvl, sq, target, TYPE(pcap));
                    }
                    else
                        add_promocap(mvl, sq, target, TYPE(pcap));
                }

            } // Right side pawn capture

        } // Pawn
        else
        {
            for (unsigned j = 0; j < num_deltas[type]; j++)
            {
                for (unsigned k = 0; k < num_squares[type]; k++)
                {
                    target = sq + (k + 1) * deltas[type][j];
                    if (OFFBOARD(target))
                        break;

                    pcap = pos.board[target];
                    if (!pcap)
                        add_quiet(mvl, sq, target);
                    else if (COLOR(pcap) != pos.side_to_move)
                    {
                        add_cap(mvl, sq, target, TYPE(pcap));
                        break;
                    }
                    else
                        break;
                }
            }

            if (type == KING)
            {
                if (pos.side_to_move == WHITE && sq == _E1)
                {
                    if ((pos.castle & WK_CASTLE) && !pos.board[_F1] && !pos.board[_G1])
                    {
                        if (!attacked(_E1, BLACK)
                            && !attacked(_F1, BLACK)
                            && !attacked(_G1, BLACK))
                        {
                            add_quiet(mvl, _E1, _G1);
                        }

                    }

                    if ((pos.castle & WQ_CASTLE) && !pos.board[_D1] && !pos.board[_C1] && !pos.board[_B1])
                    {
                        if (!attacked(_E1, BLACK)
                            && !attacked(_D1, BLACK)
                            && !attacked(_C1, BLACK))
                        {
                            add_quiet(mvl, _E1, _C1);
                        }
                    }
                }
                else if (pos.side_to_move == BLACK && sq == _E8)
                {
                    if ((pos.castle & BK_CASTLE) && !pos.board[_F8] && !pos.board[_G8])
                    {
                        if (!attacked(_E8, WHITE)
                            && !attacked(_F8, WHITE)
                            && !attacked(_G8, WHITE))
                        {
                            add_quiet(mvl, _E8, _G8);
                        }
                    }

                    if ((pos.castle & BQ_CASTLE) && !pos.board[_D8] && !pos.board[_C8] && !pos.board[_B8])
                    {
                        if (!attacked(_E8, WHITE)
                            && !attacked(_D8, WHITE)
                            && !attacked(_C8, WHITE))
                        {
                            add_quiet(mvl, _E8, _C8);
                        }
                    }
                }
            } // castle

        } // All pieces other than pawns
    }

    return mvl;
}

movelist_t* gen_captures()
{
    movelist_t* mvl = mvlsts + ply;
    mvl->num = 0;

    int sq;
    int target;
    int pcap;
    int type;
    for (unsigned i = 0; i < 16; i++)
    {
        sq = pos.pieces[pos.side_to_move][i];

        if (sq == NOSQUARE)
            continue;

        type = TYPE(pos.board[sq]);

        if (type == PAWN)
        {
            int dir = pos.side_to_move == WHITE ? UP : DOWN;

            target = sq + dir;
            if (!OFFBOARD(target))
            {
                if (pos.board[target] == 0)
                {
                    if (RANK(target) == _8 || RANK(target) == _1)
                        add_promo(mvl, sq, target);
                }
            }

            target = sq + dir + LEFT;
            if (!OFFBOARD(target))
            {
                pcap = pos.board[target];
                if ((pcap && COLOR(pcap) != pos.side_to_move)
                    || (target == pos.en_passant))
                {
                    if (RANK(target) != _8 && RANK(target) != _1)
                    {
                        if (target == pos.en_passant)
                            add_cap(mvl, sq, target, PAWN);
                        else
                            add_cap(mvl, sq, target, TYPE(pcap));
                    }
                    else
                        add_promocap(mvl, sq, target, TYPE(pcap));
                }

            } // Left side capture

            target = sq + dir + RIGHT;
            if (!OFFBOARD(target))
            {
                pcap = pos.board[target];
                if ((pcap && COLOR(pcap) != pos.side_to_move)
                    || (target == pos.en_passant))
                {
                    if (RANK(target) != _8 && RANK(target) != _1)
                    {
                        if (target == pos.en_passant)
                            add_cap(mvl, sq, target, PAWN);
                        else
                            add_cap(mvl, sq, target, TYPE(pcap));
                    }
                    else
                        add_promocap(mvl, sq, target, TYPE(pcap));
                }

            } // Right side capture
        } // pawn
        else
        {
            for (unsigned j = 0; j < num_deltas[type]; j++)
            {
                for (unsigned k = 0; k < num_squares[type]; k++)
                {
                    target = sq + (k + 1) * deltas[type][j];
                    if (OFFBOARD(target))
                        break;

                    pcap = pos.board[target];
                    if (!pcap)
                        continue;
                    else if (COLOR(pcap) != pos.side_to_move)
                    {
                        add_cap(mvl, sq, target, TYPE(pcap));
                        break;
                    }
                    else
                        break;
                }
            }
        }
    }

    return mvl;
}

/*========================================================================*//**
 * \brief Tell if a square is attacked by a piece of color "by"
 *//*=========================================================================*/
int attacked(char sq, char by)
{
    char i;
    char p;
    if (!OFFBOARD(sq + UPLEFT))
    {
        p = pos.board[sq + UPLEFT];
        if (!p)
        {
            for (i = 2; !OFFBOARD(sq + i * UPLEFT); i++)
            {
                p = pos.board[sq + i * UPLEFT];
                if (p)
                {
                    if (COLOR(p) != by)
                        break;

                    if (TYPE(p) != QUEEN && TYPE(p) != BISHOP)
                        break;

                    return 1;
                }
            }
        }
        else if (COLOR(p) == by)
        {
            if (by == BLACK && TYPE(p) == PAWN)
                return 1;

            if (TYPE(p) == KING || TYPE(p) == QUEEN || TYPE(p) == BISHOP)
                return 1;
        }
    }

    if (!OFFBOARD(sq + UPRIGHT))
    {
        p = pos.board[sq + UPRIGHT];
        if (!p)
        {
            for (i = 2; !OFFBOARD(sq + i * UPRIGHT); i++)
            {
                p = pos.board[sq + i * UPRIGHT];
                if (p)
                {
                    if (COLOR(p) != by)
                        break;

                    if (TYPE(p) != QUEEN && TYPE(p) != BISHOP)
                        break;

                    return 1;
                }
            }
        }
        else if (COLOR(p) == by)
        {
            if (by == BLACK && TYPE(p) == PAWN)
                return 1;

            if (TYPE(p) == KING || TYPE(p) == QUEEN || TYPE(p) == BISHOP)
                return 1;
        }
    }

    if (!OFFBOARD(sq + DOWNLEFT))
    {
        p = pos.board[sq + DOWNLEFT];
        if (!p)
        {
            for (i = 2; !OFFBOARD(sq + i * DOWNLEFT); i++)
            {
                p = pos.board[sq + i * DOWNLEFT];
                if (p)
                {
                    if (COLOR(p) != by)
                        break;

                    if (TYPE(p) != QUEEN && TYPE(p) != BISHOP)
                        break;

                    return 1;
                }
            }
        }
        else if (COLOR(p) == by)
        {
            if (by == WHITE && TYPE(p) == PAWN)
                return 1;

            if (TYPE(p) == KING || TYPE(p) == QUEEN || TYPE(p) == BISHOP)
                return 1;
        }
    }

    if (!OFFBOARD(sq + DOWNRIGHT))
    {
        p = pos.board[sq + DOWNRIGHT];
        if (!p)
        {
            for (i = 2; !OFFBOARD(sq + i * DOWNRIGHT); i++)
            {
                p = pos.board[sq + i * DOWNRIGHT];
                if (p)
                {
                    if (COLOR(p) != by)
                        break;

                    if (TYPE(p) != QUEEN && TYPE(p) != BISHOP)
                        break;

                    return 1;
                }
            }
        }
        else if (COLOR(p) == by)
        {
            if (by == WHITE && TYPE(p) == PAWN)
                return 1;

            if (TYPE(p) == KING || TYPE(p) == QUEEN || TYPE(p) == BISHOP)
                return 1;
        }
    }

    if (!OFFBOARD(sq + UP))
    {
        p = pos.board[sq + UP];
        if (!p)
        {
            for (i = 2; !OFFBOARD(sq + i * UP); i++)
            {
                p = pos.board[sq + i * UP];
                if (p)
                {
                    if (COLOR(p) != by)
                        break;

                    if (TYPE(p) != QUEEN && TYPE(p) != ROOK)
                        break;

                    return 1;
                }
            }
        }
        else if (COLOR(p) == by)
        {
            if (TYPE(p) == KING || TYPE(p) == QUEEN || TYPE(p) == ROOK)
                return 1;
        }
    }

    if (!OFFBOARD(sq + DOWN))
    {
        p = pos.board[sq + DOWN];
        if (!p)
        {
            for (i = 2; !OFFBOARD(sq + i * DOWN); i++)
            {
                p = pos.board[sq + i * DOWN];
                if (p)
                {
                    if (COLOR(p) != by)
                        break;

                    if (TYPE(p) != QUEEN && TYPE(p) != ROOK)
                        break;

                    return 1;
                }
            }
        }
        else if (COLOR(p) == by)
        {
            if (TYPE(p) == KING || TYPE(p) == QUEEN || TYPE(p) == ROOK)
                return 1;
        }
    }

    if (!OFFBOARD(sq + LEFT))
    {
        p = pos.board[sq + LEFT];
        if (!p)
        {
            for (i = 2; !OFFBOARD(sq + i * LEFT); i++)
            {
                p = pos.board[sq + i * LEFT];
                if (p)
                {
                    if (COLOR(p) != by)
                        break;

                    if (TYPE(p) != QUEEN && TYPE(p) != ROOK)
                        break;

                    return 1;
                }
            }
        }
        else if (COLOR(p) == by)
        {
            if (TYPE(p) == KING || TYPE(p) == QUEEN || TYPE(p) == ROOK)
                return 1;
        }
    }

    if (!OFFBOARD(sq + RIGHT))
    {
        p = pos.board[sq + RIGHT];
        if (!p)
        {
            for (i = 2; !OFFBOARD(sq + i * RIGHT); i++)
            {
                p = pos.board[sq + i * RIGHT];
                if (p)
                {
                    if (COLOR(p) != by)
                        break;

                    if (TYPE(p) != QUEEN && TYPE(p) != ROOK)
                        break;

                    return 1;
                }
            }
        }
        else if (COLOR(p) == by)
        {
            if (TYPE(p) == KING || TYPE(p) == QUEEN || TYPE(p) == ROOK)
                return 1;
        }
    }

    if (!OFFBOARD(sq + UPLEFT2))
    {
        if (TYPE(pos.board[sq + UPLEFT2]) == KNIGHT
            && COLOR(pos.board[sq + UPLEFT2]) == by)
            return 1;
    }

    if (!OFFBOARD(sq + UP2LEFT))
    {
        if (TYPE(pos.board[sq + UP2LEFT]) == KNIGHT
            && COLOR(pos.board[sq + UP2LEFT]) == by)
            return 1;
    }

    if (!OFFBOARD(sq + UP2RIGHT))
    {
        if (TYPE(pos.board[sq + UP2RIGHT]) == KNIGHT
            && COLOR(pos.board[sq + UP2RIGHT]) == by)
            return 1;
    }

    if (!OFFBOARD(sq + UPRIGHT2))
    {
        if (TYPE(pos.board[sq + UPRIGHT2]) == KNIGHT
            && COLOR(pos.board[sq + UPRIGHT2]) == by)
            return 1;
    }

    if (!OFFBOARD(sq + UPLEFT2))
    {
        if (TYPE(pos.board[sq + UPLEFT2]) == KNIGHT
            && COLOR(pos.board[sq + UPLEFT2]) == by)
            return 1;
    }

    if (!OFFBOARD(sq + UP2LEFT))
    {
        if (TYPE(pos.board[sq + UP2LEFT]) == KNIGHT
            && COLOR(pos.board[sq + UP2LEFT]) == by)
            return 1;
    }

    if (!OFFBOARD(sq + UP2RIGHT))
    {
        if (TYPE(pos.board[sq + UP2RIGHT]) == KNIGHT
            && COLOR(pos.board[sq + UP2RIGHT]) == by)
            return 1;
    }

    if (!OFFBOARD(sq + UPRIGHT2))
    {
        if (TYPE(pos.board[sq + UPRIGHT2]) == KNIGHT
            && COLOR(pos.board[sq + UPRIGHT2]) == by)
            return 1;
    }

    if (!OFFBOARD(sq + DOWNLEFT2))
    {
        if (TYPE(pos.board[sq + DOWNLEFT2]) == KNIGHT
            && COLOR(pos.board[sq + DOWNLEFT2]) == by)
            return 1;
    }

    if (!OFFBOARD(sq + DOWN2LEFT))
    {
        if (TYPE(pos.board[sq + DOWN2LEFT]) == KNIGHT
            && COLOR(pos.board[sq + DOWN2LEFT]) == by)
            return 1;
    }

    if (!OFFBOARD(sq + DOWN2RIGHT))
    {
        if (TYPE(pos.board[sq + DOWN2RIGHT]) == KNIGHT
            && COLOR(pos.board[sq + DOWN2RIGHT]) == by)
            return 1;
    }

    if (!OFFBOARD(sq + DOWNRIGHT2))
    {
        if (TYPE(pos.board[sq + DOWNRIGHT2]) == KNIGHT
            && COLOR(pos.board[sq + DOWNRIGHT2]) == by)
            return 1;
    }

    return 0;
}

void add_quiet(movelist_t* mvl, int from, int to)
{
    mvl->moves[mvl->num].b.from = from;
    mvl->moves[mvl->num].b.to = to;
    mvl->moves[mvl->num].b.promotion = 0;
    mvl->moves[mvl->num].b.bits = 0;
    mvl->score[mvl->num] = heuristic_his[from][to];
    if (killers[pos.side_to_move][from][to])
    {
        mvl->score[mvl->num]
            += SORT_KILLER + killers[pos.side_to_move][from][to];
    }
    mvl->num++;
}

void add_promo(movelist_t* mvl, int from, int to)
{
    for (unsigned j = 0; j < 4; j++)
    {
        mvl->moves[mvl->num].b.from = from;
        mvl->moves[mvl->num].b.to = to;
        mvl->moves[mvl->num].b.promotion = j + KNIGHT;
        mvl->moves[mvl->num].b.bits = 0;
        mvl->score[mvl->num] = SORT_PROMOTION + piece_value[j + KNIGHT];
        mvl->num++;
    }
}

void add_cap(movelist_t* mvl, int from, int to, int captype)
{
    mvl->moves[mvl->num].b.from = from;
    mvl->moves[mvl->num].b.to = to;
    mvl->moves[mvl->num].b.promotion = 0;
    mvl->moves[mvl->num].b.bits = 0;
    mvl->score[mvl->num] = SORT_CAPTURE
        + 10 * piece_value[captype]
        - piece_value[TYPE(pos.board[from])];
    mvl->num++;
}

void add_promocap(movelist_t* mvl, int from, int to, int captype)
{
    for (unsigned j = 0; j < 4; j++)
    {
        mvl->moves[mvl->num].b.from = from;
        mvl->moves[mvl->num].b.to = to;
        mvl->moves[mvl->num].b.promotion = j + KNIGHT;
        mvl->moves[mvl->num].b.bits = 0;
        mvl->score[mvl->num] = SORT_CAPTURE
            + 10 * piece_value[captype]
            + piece_value[j + KNIGHT];
        mvl->num++;
    }
}
