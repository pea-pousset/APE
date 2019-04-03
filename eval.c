#include "eval.h"
#include "position.h"
#include "transposition.h"

#define ENDGAME_MATERIAL_THRESHOLD  1300

//#define PASSED_PAWN_BONUS               +15
//#define PASSED_PAWN_BONUS_FINAL         +20
#define PASSED_PAWN_PER_RANK_BONUS          +22
#define PASSED_PAWN_IS_PROTECTED_BONUS      +13
#define BACKWARD_PAWN_MALUS                 -20
#define ISOLATED_PAWN_MALUS                  -8
#define DOUBLE_PAWN_MALUS                   -12
#define MULTIPLE_DOUBLE_PAWN_MALUS          -15
#define TRIPLE_PAWN_MALUS                   -35

#define ROOK_ON_SEMIOPEN_BONUS               15
#define ROOK_ON_OPEN_BONUS                   20
#define ROOK_ON_SEVENTH_BONUS                20

#define EARLY_QUEEN_MALUS                    -6 // per non developed minor piece

#define CANNOT_CASTLE_MALUS                 -27

/**
 * Pieces base score in centipawns
 */
const int piece_value[7] = {0, 100, 300, 320, 500, 975, 10000 };

/**
 * Correcting factor for the pieces value based on the number of own pawns.
 * The bishops "automatically" become stronger than the knights when there
 * are few pawns.
 */
const int pval_bypawn[9][7] =
{
    //      P    N    B    R    Q    K
    {  0,   0, -20,   0, +15,   0,   0 }, // no pawn
    {  0,   0, -15,   0, +12,   0,   0 }, // 1 pawn
    {  0,   0, -10,   0,  +8,   0,   0 },
    {  0,   0,  -5,   0,  +5,   0,   0 },
    {  0,   0,  -2,   0,  +2,   0,   0 },
    {  0,   0,   0,   0,   0,   0,   0 },
    {  0,   0,   5,   0,  -5,   0,   0 },
    {  0,   0,  10,   0, -10,   0,   0 },
    {  0,   0, +15,   0, -15,   0,   0 }
};

/**
 * Malus points for pawns not protecting the king for they are too advanced.
 * We use the "pla" table so the 8th rank means there's no pawn
 */
const int pawn_shield[128] =
{
   -30,-30,-25,-10,-10,-25,-30,-30,   -30,-30,-25,-10,-10,-25,-30,-30,
   -25,-25,-20,  0,  0,-20,-25,-25,   -25,-25,-20,  0,  0,-20,-25,-25,
   -25,-25,-20,  0,  0,-20,-25,-25,   -25,-25,-20,  0,  0,-20,-25,-25,
   -20,-20,-15,  0,  0,-15,-20,-20,   -20,-20,-15,  0,  0,-15,-20,-20,
   -15,-15,-10,  0,  0,-10,-15,-15,   -15,-15,-10,  0,  0,-10,-15,-15,
    -7, -7, -5,  0,  0, -5, -7, -7,    -7, -7, -5,  0,  0, -5, -7, -7,
     0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0
};

const int piece_squares[7][128] =
{
    { 0 },

    {
    /*   PAWN MIDDLEGAME
         A   B   C   D   E   F   G   H      H   G   F   E   D   C   B   A
        ------------------------------     ------------------------------
   1 */  0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, /* 8
   2 */  5,  7,  9,  9,  9,  9,  7,  5,     5,  7,  9,  9,  9,  9,  7,  5, /* 7
   3 */  4,  6,  7,  9,  9,  7,  6,  4,     4,  6,  7,  9,  9,  7,  6,  4, /* 6
   4 */  0,  0,  4,  8,  8,  4,  0,  0,     0,  0,  4,  8,  8,  4,  0,  0, /* 5
   5 */  0,  0,  6, 10, 10,  6,  0,  0,     0,  0,  6, 10, 10,  6,  0,  0, /* 4
   6 */  2,  0,  3,  3,  3,  3,  0,  2,     2,  0,  3,  3,  3,  3,  0,  2, /* 3
   7 */  0,  0,  0,-25,-25,  0,  0,  0,     0,  0,  0,-25,-25,  0,  0,  0, /* 2
   8 */  0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0  /* 1
                     BLACK                               WHITE              */
    },
    // KNIGHT
    {
        -8, -8, -8, -8, -8, -8, -8, -8,    -8, -8, -8, -8, -8, -8, -8, -8,
        -8,  0,  0,  0,  0,  0,  0, -8,    -8,  0,  0,  0,  0,  0,  0, -8,
        -8,  2,  4,  4,  4,  4,  2, -8,    -8,  2,  4,  4,  4,  4,  2, -8,
        -8,  1,  3,  6,  6,  3,  1, -8,    -8,  1,  3,  6,  6,  3,  1, -8,
        -8,  0,  3,  6,  6,  3,  0, -8,    -8,  0,  3,  6,  6,  3,  0, -8,
        -8,  0,  3,  6,  6,  3,  0, -8,    -8,  0,  3,  6,  6,  3,  0, -8,
        -8,  0,  0,  0,  0,  0,  0, -8,    -8,  0,  0,  0,  0,  0,  0, -8,
       -18,-13, -8, -8, -8, -8,-13,-18,   -18,-13, -8, -8, -8, -8,-13,-18,
    },
    // BISHOP
    {
        -5, -5, -5, -5, -5, -5, -5, -5,    -5, -5, -5, -5, -5, -5, -5, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,    -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  3,  5,  5,  3,  0, -5,    -5,  0,  3,  5,  5,  3,  0, -5,
        -5,  0,  5,  7,  7,  5,  0, -5,    -5,  0,  5,  7,  7,  5,  0, -5,
        -5,  0,  5,  7,  7,  5,  0, -5,    -5,  0,  5,  7,  7,  5,  0, -5,
        -5,  2,  3,  5,  5,  3,  2, -5,    -5,  2,  3,  5,  5,  3,  2, -5,
        -5,  4,  2,  2,  2,  2,  4, -5,    -5,  4,  2,  2,  2,  2,  4, -5,
        -5, -5,-11, -5, -5, -11,-5, -5,    -5, -5,-11, -5, -5, -11,-5, -5,
    },
    // ROOK
    {
        -3, -3, -3, -3, -3, -3, -3, -3,    -3, -3, -3, -3, -3, -3, -3, -3,
        -3,  0,  0,  0,  0,  0,  0, -3,    -3,  0,  0,  0,  0,  0,  0, -3,
        -3,  0,  0,  0,  0,  0,  0, -3,    -3,  0,  0,  0,  0,  0,  0, -3,
        -3,  0,  0,  0,  0,  0,  0, -3,    -3,  0,  0,  0,  0,  0,  0, -3,
        -3,  0,  0,  0,  0,  0,  0, -3,    -3,  0,  0,  0,  0,  0,  0, -3,
        -3,  0,  0,  0,  0,  0,  0, -3,    -3,  0,  0,  0,  0,  0,  0, -3,
        -3,  0,  0,  0,  0,  0,  0, -3,    -3,  0,  0,  0,  0,  0,  0, -3,
         0,  0,  0,  3,  3,  0,  0, -3,     0,  0,  0,  3,  3,  0,  0,  0
    },
    // QUEEN
    {
         0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  1,  1,  1,  1,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  1,  2,  2,  1,  0,  0,     0,  0,  1,  2,  2,  1,  0,  0,
         0,  0,  2,  3,  3,  2,  0,  0,     0,  0,  2,  3,  3,  2,  0,  0,
         0,  0,  2,  3,  3,  2,  0,  0,     0,  0,  2,  3,  3,  2,  0,  0,
         0,  0,  1,  2,  2,  1,  0,  0,     0,  0,  1,  2,  2,  1,  0,  0,
         0,  0,  1,  1,  1,  1,  0,  0,     0,  0,  1,  1,  1,  1,  0,  0,
        -5, -5, -5, -5, -5, -5, -5, -5,    -5, -5, -5, -5, -5, -5, -5, -5
    },
    {
        -60,-60,-60,-60,-60,-60,-60,-60,   -60,-60,-60,-60,-60,-60,-60,-60,
        -60,-60,-60,-60,-60,-60,-60,-60,   -60,-60,-60,-60,-60,-60,-60,-60,
        -60,-60,-60,-60,-60,-60,-60,-60,   -60,-60,-60,-60,-60,-60,-60,-60,
        -60,-60,-60,-60,-60,-60,-60,-60,   -60,-60,-60,-60,-60,-60,-60,-60,
        -60,-60,-60,-60,-60,-60,-60,-60,   -60,-60,-60,-60,-60,-60,-60,-60,
        -40,-40,-40,-40,-40,-40,-40,-40,   -40,-40,-40,-40,-40,-40,-40,-40,
        -10,  0,-20,-20,-20,-20,  0,-10,   -10,  0,-20,-20,-20,-20,  0,-10,
          0, 20, 40,-20,  0,-20, 40, 20,    20, 40,-20,  0,-20, 40, 20,  0
    }

};

const int king_endgame_sq[128] =
{
	-60,-50,-40,-30,-30,-40,-50,-60,      -60,-50,-40,-30,-30,-40,-50,-60,
	-50,-30,-15,  8,  8,-15,-30,-50,      -50,-30,-15,  8,  8,-15,-30,-50,
	-40,-15,  8, 10, 10,  8,-15,-40,      -40,-15,  8, 10, 10,  8,-15,-40,
	-30,  8, 10, 12, 12, 10,  8,-30,      -30,  8, 10, 12, 12, 10,  8,-30,
	-30,  8, 10, 12, 12, 10,  8,-30,      -30,  8, 10, 12, 12, 10,  8,-30,
	-40,-15,  8, 10, 10,  8,-15,-40,      -40,-15,  8, 10, 10,  8,-15,-40,
	-50,-30,-15,  8,  8,-15,-30,-50,      -50,-30,-15,  8,  8,-15,-30,-50,
	-60,-50,-40,-30,-30,-40,-50,-60,      -60,-50,-40,-30,-30,-40,-50,-60,
};

int is_protected(int sq, int by);

int eval()
{
    int total_score;
    int pawns_score;
    if (tteval_probe(&total_score))
        return total_score;

    int score[2] = { 0 };
    int pawn_count[2] = { 0 };
    int piece_count[2] = { 0 };
    int piece_material[2] = { 0 };
    int pawn_per_file[2][8] = { 0 };
    int pawn_per_rank[2][8] = { 0 };
    int fendgamew = 0;    // white is in endgame stage
    int fendgameb = 0;
    int wndpawns = 0;  // number of white double pawns
    int bndpawns = 0;

    // most advanced pawn for each file and each color
    int pma[2][8] = { {0}, {7, 7, 7, 7, 7, 7, 7, 7} };

    // least advanced pawn for each file and each color
    int pla[2][8] = { {7, 7, 7, 7, 7, 7, 7, 7}, {0} };

    piece_t bp, wp;

    // First loop to gather informations on material and pawn structure
    for (int i = 0; i < 16; i++)
    {
        wp = pos.board[pos.pieces[WHITE][i]];
        bp = pos.board[pos.pieces[BLACK][i]];
        if (wp)
        {
            int file = FILE(pos.pieces[WHITE][i]);
            int rank = RANK(pos.pieces[WHITE][i]);
            if (TYPE(wp) == PAWN)
            {
                ++pawn_per_file[WHITE][file];
                ++pawn_per_rank[WHITE][rank];
                ++pawn_count[WHITE];

                if (rank > pma[WHITE][file])
                    pma[WHITE][file] = rank;

                if (rank < pla[WHITE][file])
                    pla[WHITE][file] = rank;
            }
            else if (TYPE(wp) != KING)
            {
                ++piece_count[WHITE];
                piece_material[WHITE] += piece_value[TYPE(wp)];
            }
        }

        if (bp)
        {
            int file = FILE(pos.pieces[BLACK][i]);
            int rank = RANK(pos.pieces[BLACK][i]);
            if (TYPE(bp) == PAWN)
            {
                ++pawn_per_file[BLACK][file];
                ++pawn_per_rank[BLACK][file];
                ++pawn_count[BLACK];

                if (rank < pma[BLACK][file])
                    pma[BLACK][file] = rank;

                if (rank > pla[BLACK][file])
                    pla[BLACK][file] = rank;
            }
            else if (TYPE(bp) != KING)
            {
                ++piece_count[BLACK];
                piece_material[BLACK] += piece_value[TYPE(bp)];
            }
        }
    }

    if (piece_material[WHITE] < ENDGAME_MATERIAL_THRESHOLD)
        fendgamew = 1;
    if (piece_material[BLACK] < ENDGAME_MATERIAL_THRESHOLD)
        fendgameb = 1;

    // Is white ahead of material ?
/*    if ( (piece_material[WHITE] > piece_material[BLACK]
          && pawn_count[WHITE] >= pawn_count[BLACK])
        ||
         (piece_material[WHITE] == piece_material[BLACK]
          && pawn_count[WHITE] > pawn_count[BLACK])
        )
    {
        // Discourage black to trade down
        score[BLACK] += (7 - piece_count[BLACK]) * -6;
        score[BLACK] += (8 - pawn_count[BLACK]) * -4;
    }
    else if ( (piece_material[BLACK] > piece_material[WHITE]
          && pawn_count[BLACK] >= pawn_count[WHITE])
        ||
         (piece_material[BLACK] == piece_material[WHITE]
          && pawn_count[BLACK] > pawn_count[WHITE])
        )
    {
        score[WHITE] += (7 - piece_count[WHITE]) * -6;
        score[WHITE] += (8 - pawn_count[WHITE]) * -4;
    }
*/


    if (!ttpawn_probe(&pawns_score))
    {
        // Pawn structure evaluation. It decomposes the board into 3 files
        // large chunks and evaluate the middle file pawns based on the
        // two adjacent ones.

        int ps[2] = { 0 };
        for (unsigned i = 0; i < 8; ++i)
        {
            int fpassedw = 0;   // passed white pawn flag to deactivate
            int fpassedb = 0;   // some maluses

            // Any white pawn on file i ?
            if (pawn_per_file[WHITE][i])
            {
                // Most advanced pawn is not opposed ?
                if (pla[BLACK][i] < pma[WHITE][i])
                {
                    // Not defended ?
                    if (   (i > 0 ? pla[BLACK][i-1] <= pma[WHITE][i] : 1)
                        && (i < 7 ? pla[BLACK][i+1] <= pma[WHITE][i] : 1) )
                    {
                        fpassedw = 1;
                        ps[WHITE] += pma[WHITE][i] * PASSED_PAWN_PER_RANK_BONUS;
                        if (is_protected(SQUARE(i, pma[WHITE][i]), WHITE))
                        //{
                            // printf("protected ");
                            ps[WHITE] += PASSED_PAWN_IS_PROTECTED_BONUS;
                        //}
                        //printf("passed white pawn on file %c\n", i + 'A');
                    }
                }

                // Least advanced pawn's front square is attacked ?
                if (!pos.board[SQUARE(i, pla[WHITE][i]) + UP])
                {
                    if (is_protected(SQUARE(i, pla[WHITE][i]+UP), BLACK))
                    {
                        ps[WHITE] += BACKWARD_PAWN_MALUS;
                    }
                }


                // No friend pawn on adjacent files ?
                if (   (i > 0 ? !pawn_per_file[WHITE][i-1] : 1)
                    && (i < 7 ? !pawn_per_file[WHITE][i+1] : 1)
                    && !fpassedw )
                {
                    ps[WHITE] += ISOLATED_PAWN_MALUS;
                    // printf("isolated white pawn on file %c\n", i + 'A');
                }
            }

            if (pawn_per_file[BLACK][i])
            {
                if (pma[BLACK][i] < 7 && pla[WHITE][i] > pma[BLACK][i])
                {
                    if (   (i > 0 ? pla[WHITE][i-1] >= pma[BLACK][i] : 1)
                        && (i < 7 ? pla[WHITE][i+1] >= pma[BLACK][i] : 1) )
                    {
                        fpassedb = 1;
                        ps[BLACK] += (6 - pma[BLACK][i]) * PASSED_PAWN_PER_RANK_BONUS;

                        if (is_protected(SQUARE(i, pma[BLACK][i]), BLACK))
                        //{
                            //printf("protected ");
                            ps[BLACK] += PASSED_PAWN_IS_PROTECTED_BONUS;
                        //}
                        // printf("passed black pawn on file %c\n", i + 'A');
                    }
                }

                if (!pos.board[SQUARE(i, pla[BLACK][i]) + DOWN])
                {
                    if (is_protected(SQUARE(i, pla[BLACK][i]) + DOWN, WHITE))
                    {
                        ps[BLACK] += BACKWARD_PAWN_MALUS;
                    }
                }

                if (   (i > 0 ? !pawn_per_file[BLACK][i-1] : 1)
                    && (i < 7 ? !pawn_per_file[BLACK][i+1] : 1)
                    && !fpassedb )
                {
                    ps[BLACK] += ISOLATED_PAWN_MALUS;
                    // printf("isolated black pawn on file %c\n", i + 'A');
                }
            }


            if (pawn_per_file[WHITE][i] == 2 && !fpassedw)
            {
                // printf("doubled white pawns on file %c\n", i + 'A');
                ps[WHITE] += DOUBLE_PAWN_MALUS;
                wndpawns++;
            }
            else if (pawn_per_file[WHITE][i] == 3)
            {
                // printf("tripled white pawns on file %c\n", i + 'A');
                ps[WHITE] += TRIPLE_PAWN_MALUS;
                wndpawns++;
            }

            if (pawn_per_file[BLACK][i] == 2 && !fpassedb)
            {
                // printf("doubled black pawns on file %c\n", i + 'A');
                ps[BLACK] += DOUBLE_PAWN_MALUS;
                bndpawns++;
            }
            else if (pawn_per_file[BLACK][i] == 3)
            {
                // printf("tripled white pawns on file %c\n", i + 'A');
                ps[BLACK] += TRIPLE_PAWN_MALUS;
                bndpawns++;
            }
        }

        ps[WHITE] += wndpawns * MULTIPLE_DOUBLE_PAWN_MALUS;
        ps[BLACK] += bndpawns * MULTIPLE_DOUBLE_PAWN_MALUS;

        if (pos.side_to_move == WHITE)
            pawns_score = ps[WHITE] - ps[BLACK];
        else
            pawns_score = ps[BLACK] - ps[WHITE];

        ttpawn_save(pawns_score);
    }
    /*==================== end of pawn structure eval ========================*/

    for (int i = 0; i < 16; i++)
    {
        wp = pos.board[pos.pieces[WHITE][i]];
        bp = pos.board[pos.pieces[BLACK][i]];

        if (wp)
        {
            int file = FILE(pos.pieces[WHITE][i]);
            int rank = RANK(pos.pieces[WHITE][i]);
            int type = TYPE(wp);

            score[WHITE] += piece_value[type];
            score[WHITE] += pval_bypawn[pawn_count[WHITE]][type];

            if (type != KING)
            {
                score[WHITE] += piece_squares[type][127 - pos.pieces[WHITE][i]];

                if (type == ROOK)
                {
                    if (!pawn_per_file[WHITE][file])
                    {
                        if (pawn_per_file[BLACK][file])
                            score[WHITE] += ROOK_ON_SEMIOPEN_BONUS;
                        else
                            score[WHITE] += ROOK_ON_OPEN_BONUS;
                    }

                    if (rank == _7 && (pawn_per_rank[BLACK][_7]
                        || RANK(pos.king_square[BLACK]) == _8)) // Help to lock the enemy king
                    {
                        score[WHITE] += ROOK_ON_SEVENTH_BONUS;
                    }
                }
            }
            else
            {
                if (fendgamew)
                    score[WHITE] += king_endgame_sq[127 - pos.pieces[WHITE][i]];
                else
                    score[WHITE] += piece_squares[KING][127 - pos.pieces[WHITE][i]];

                if (WHITE_CANNOT_CASTLE() && !pos.has_castled[WHITE])
                    score[WHITE] += CANNOT_CASTLE_MALUS;

                if (rank <= _2)
                {
                    int ks = 0;
                    if (file < _E)
                    {
                        ks += pawn_shield[127 - SQUARE(_A, pla[WHITE][_A])];
                        ks += pawn_shield[127 - SQUARE(_B, pla[WHITE][_B])];
                        ks += pawn_shield[127 - SQUARE(_C, pla[WHITE][_C])];
                    }
                    else if (file > _E)
                    {
                        ks += pawn_shield[127 - SQUARE(_F, pla[WHITE][_F])];
                        ks += pawn_shield[127 - SQUARE(_G, pla[WHITE][_G])];
                        ks += pawn_shield[127 - SQUARE(_H, pla[WHITE][_H])];
                    }
                    else
                    {
                        ks += pawn_shield[127 - SQUARE(_E, pla[WHITE][_D])];
                        ks += pawn_shield[127 - SQUARE(_E, pla[WHITE][_E])];
                        ks += pawn_shield[127 - SQUARE(_E, pla[WHITE][_F])];
                    }
                    ks *= piece_material[BLACK];
                    ks /= 3215;
                    score[WHITE] += ks;
                }
            }
        }

        if (bp)
        {
            int file = FILE(pos.pieces[BLACK][i]);
            int rank = RANK(pos.pieces[BLACK][i]);
            int type = TYPE(bp);

            score[BLACK] += piece_value[type];
            score[BLACK] += pval_bypawn[pawn_count[BLACK]][type];

            if (type != KING)
            {
                score[BLACK] += piece_squares[type][pos.pieces[BLACK][i]];

                if (type == ROOK)
                {
                    if (!pawn_per_file[BLACK][file])
                    {
                        if (pawn_per_file[WHITE][file])
                            score[BLACK] += ROOK_ON_SEMIOPEN_BONUS;
                        else
                            score[BLACK] += ROOK_ON_OPEN_BONUS;
                    }

                    if (rank == _2 && (pawn_per_rank[WHITE][_2]
                        || RANK(pos.king_square[WHITE]) == _1))
                    {
                        score[BLACK] += ROOK_ON_SEVENTH_BONUS;
                    }
                }
            }
            else
            {
                if (fendgameb)
                    score[BLACK] += king_endgame_sq[pos.pieces[BLACK][i]];
                else
                    score[BLACK] += piece_squares[KING][pos.pieces[BLACK][i]];


                if (BLACK_CANNOT_CASTLE() && !pos.has_castled[BLACK])
                    score[BLACK] += CANNOT_CASTLE_MALUS;

                if (rank >= _7)
                {
                    int ks = 0;
                    if (file < _E)
                    {
                        ks += pawn_shield[SQUARE(_A, pla[BLACK][_A])];
                        ks += pawn_shield[SQUARE(_B, pla[BLACK][_B])];
                        ks += pawn_shield[SQUARE(_C, pla[BLACK][_C])];
                    }
                    else if (file > _E)
                    {
                        ks += pawn_shield[SQUARE(_F, pla[BLACK][_H])];
                        ks += pawn_shield[SQUARE(_G, pla[BLACK][_G])];
                        ks += pawn_shield[SQUARE(_H, pla[BLACK][_F])];
                    }
                    else
                    {
                        ks += pawn_shield[SQUARE(_E, pla[BLACK][_D])];
                        ks += pawn_shield[SQUARE(_E, pla[BLACK][_E])];
                        ks += pawn_shield[SQUARE(_E, pla[BLACK][_F])];
                    }
                    ks *= piece_material[WHITE];
                    ks /= 3215;
                    score[BLACK] += ks;
                }
            }
        }
    }

    // If the queen is not on its starting square, give a malus for each
    // undeveloped piece.
    if (!pos.board[_D1])
    {
        if (COLOR(pos.board[_B1]) == WHITE && TYPE(pos.board[_B1]) == KNIGHT)
            score[WHITE] += EARLY_QUEEN_MALUS;
        if (COLOR(pos.board[_C1]) == WHITE && TYPE(pos.board[_C1]) == BISHOP)
            score[WHITE] += EARLY_QUEEN_MALUS;
        if (COLOR(pos.board[_F1]) == WHITE && TYPE(pos.board[_F1]) == BISHOP)
            score[WHITE] += EARLY_QUEEN_MALUS;
        if (COLOR(pos.board[_G1]) == WHITE && TYPE(pos.board[_G1]) == KNIGHT)
            score[WHITE] += EARLY_QUEEN_MALUS;
    }

    if (!pos.board[_D8])
    {
        if (COLOR(pos.board[_B8]) == BLACK && TYPE(pos.board[_B8]) == KNIGHT)
            score[BLACK] += EARLY_QUEEN_MALUS;
        if (COLOR(pos.board[_C8]) == BLACK && TYPE(pos.board[_C8]) == BISHOP)
            score[BLACK] += EARLY_QUEEN_MALUS;
        if (COLOR(pos.board[_F8]) == BLACK && TYPE(pos.board[_F8]) == BISHOP)
            score[BLACK] += EARLY_QUEEN_MALUS;
        if (COLOR(pos.board[_G8]) == BLACK && TYPE(pos.board[_G8]) == KNIGHT)
            score[BLACK] += EARLY_QUEEN_MALUS;
    }



    if (pos.side_to_move == WHITE)
        total_score = score[WHITE] - score[BLACK];
    else
        total_score = score[BLACK] - score[WHITE];

    total_score += pawns_score;

    tteval_save(total_score);
    return total_score;
}

/*========================================================================*//**
 * \brief Tell whether the square 'sq' is protected by a pawn of color 'by'
 *//*=========================================================================*/
int is_protected(int sq, int by)
{
    int dir = by == WHITE ? DOWN : UP;

    if (!OFFBOARD(sq + dir + LEFT))
    {
        int p = pos.board[sq + dir + LEFT];
        if (COLOR(p) == by && TYPE(p) == PAWN)
            return 1;
    }

    if (!OFFBOARD(sq + dir + RIGHT))
    {
        int p = pos.board[sq + dir + RIGHT];
        if (COLOR(p) == by && TYPE(p) == PAWN)
            return 1;
    }
    return 0;
}
