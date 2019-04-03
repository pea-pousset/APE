#ifndef PIECE_H
#define PIECE_H

#include <stdint.h>

//  +---+---+---+---+---+---+---+---+
//  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//  +---+---+---+---+---+---+---+---+
//  |     Index     |   Type    | C |
//  +---+---+---+---+---+---+---+---+
//  C : color

#define WHITE       0
#define BLACK       1

#define PAWN        1
#define KNIGHT      2
#define BISHOP      3
#define ROOK        4
#define QUEEN       5
#define KING        6

#define COLOR(x)    (x & 0x01)
#define TYPE(x)     ((x & 0x0E) >> 1)
#define INDEX(x)    ((x & 0xF0) >> 4)

#define PIECE(color, type, index)  (color | (type << 1) | (index << 4))

typedef uint8_t piece_t;

#endif
