#ifndef BOARD_H
#define BOARD_H

#include "piece.h"

// 0x88 board

#define FILE(x)             ((x) & 7)
#define RANK(x)             ((x) >> 4)
#define SQUARE(file, rank)  (16 * (rank) + (file))
#define OFFBOARD(x)         ((x) & 0x88)

#define NOSQUARE            127

enum
{
    _A1 = 0,   _B1, _C1, _D1, _E1, _F1, _G1, _H1,
    _A2 = 16,  _B2, _C2, _D2, _E2, _F2, _G2, _H2,
    _A3 = 32,  _B3, _C3, _D3, _E3, _F3, _G3, _H3,
    _A4 = 48,  _B4, _C4, _D4, _E4, _F4, _G4, _H4,
    _A5 = 64,  _B5, _C5, _D5, _E5, _F5, _G5, _H5,
    _A6 = 80,  _B6, _C6, _D6, _E6, _F6, _G6, _H6,
    _A7 = 96,  _B7, _C7, _D7, _E7, _F7, _G7, _H7,
    _A8 = 112, _B8, _C8, _D8, _E8, _F8, _G8, _H8
};

// Ranks
enum
{
    _1, _2, _3, _4, _5, _6, _7, _8
};

// Files
enum
{
    _A, _B, _C, _D, _E, _F, _G, _H
} ;

typedef piece_t board_t[128];

#endif
