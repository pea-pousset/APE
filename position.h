#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include "board.h"
#include "move.h"

#define WK_CASTLE   0x01
#define WQ_CASTLE   0x02
#define BK_CASTLE   0x04
#define BQ_CASTLE   0x08

#define W_CASTLES   (WK_CASTLE | WQ_CASTLE)
#define B_CASTLES   (BK_CASTLE | BQ_CASTLE)

#define WHITE_CANNOT_CASTLE() (!(pos.castle & W_CASTLES))
#define BLACK_CANNOT_CASTLE() (!(pos.castle & B_CASTLES))

typedef struct
{
    board_t  board;
    int      pieces[2][16];
    int      side_to_move;
    int      en_passant;
    int      castle;
    int      king_square[2];
    int      has_castled[2];
    int      half_moves;
    int      nmove;
    uint64_t hash;
    uint64_t pawn_hash;
} position_t;

typedef struct
{
    move_t   move;
    piece_t  capture;
    int      en_passant;
    int      castle;
    int      half_moves;
    uint64_t hash;
    uint64_t pawn_hash;
} history_t;

extern position_t pos;
extern history_t* his;

void   print();
void   reset();
int    set_fen(const char* fen);
void   make_move(move_t move);
void   unmake_move();
move_t last_move();
int    in_check(int side);
int    illegal();
int    move_played();
int    is_repetition();


#endif
