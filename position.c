#include "position.h"
#include "utils.h"
#include "search.h"
#include "transposition.h"
#include "errors.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

position_t pos;

static history_t history_entries[2048];
history_t* his;


static void clear();

/*========================================================================*//**
 * \brief Affiche l'echiquier dans la console
 *//*=========================================================================*/
void print()
{
    for (int r = _8; r >= _1; r--)
    {
        printf("   +---+---+---+---+---+---+---+---+\n");
        printf(" %d ", r + 1);
        for (int f = _A; f <= _H; f++)
        {
            char c = " pnbrqk"[TYPE(pos.board[SQUARE(f,r)])];
            char d = ' ';
            char e = ' ';
            if (pos.board[SQUARE(f,r)] && COLOR(pos.board[SQUARE(f,r)]) == WHITE)
            {
                c -= 'a' - 'A';
                d = '(';
                e = ')';
            }
            printf("|%c%c%c", d, c, e);
        }
        printf("| ");

        if (r == _8)
        {
            if (pos.side_to_move == WHITE)
                printf("%d. White to move", pos.nmove);
            else
                printf("%d. Black to move", pos.nmove);
        }
        else if (r == _7)
        {
            printf("Castle: %c%c%c%c",
                   (pos.castle & WK_CASTLE) ? 'K' : '-',
                   (pos.castle & WQ_CASTLE) ? 'Q' : '-',
                   (pos.castle & BK_CASTLE) ? 'k' : '-',
                   (pos.castle & BQ_CASTLE) ? 'q' : '-');
        }
        else if (r == _6)
        {
            if (pos.en_passant != NOSQUARE)
                printf("En passant: %c%c", FILE(pos.en_passant) + 'a',
                       RANK(pos.en_passant) + '1');
        }
        else if (r == _5)
        {
            printf("Draw in %d moves",
                   (100 - pos.half_moves) / 2 + (100 - pos.half_moves) % 2);
        }
        else if (r == _2)
        {
            printf("%"PRIX64, pos.pawn_hash);
        }
        else if (r == _1)
        {
            printf("%"PRIX64, pos.hash);
        }

        printf("\n");
    }
    printf("   +---+---+---+---+---+---+---+---+\n");
    printf("     A   B   C   D   E   F   G   H \n\n");
}

/*========================================================================*//**
 * \brief Met les pieces dans la position initiale
 *//*=========================================================================*/
void reset()
{
    clear();
    int color, type;
    int bpi = 0, wpi = 0;
    for (unsigned i = 0; i < 128; i++)
    {
        if (OFFBOARD(i))
        {
            i += 7;
            continue;
        }

        if (RANK(i) > _2 && RANK(i) < _7)
            continue;

        if (RANK(i) == _2)
        {
            pos.hash ^= zobrist_pieces[WHITE][PAWN][i];
            pos.pawn_hash ^= zobrist_pieces[WHITE][PAWN][i];
            pos.board[i] = PIECE(WHITE, PAWN, wpi);
            pos.pieces[WHITE][wpi] = i;
            ++wpi;
            continue;
        }
        else if (RANK(i) == _7)
        {
            pos.hash ^= zobrist_pieces[BLACK][PAWN][i];
            pos.pawn_hash ^= zobrist_pieces[BLACK][PAWN][i];
            pos.board[i] = PIECE(BLACK, PAWN, bpi);
            pos.pieces[BLACK][bpi] = i;
            ++bpi;
            continue;
        }
        else if (RANK(i) == _1)
        {
            color = WHITE;
        }
        else
        {
            color = BLACK;
        }

        if (FILE(i) == _A || FILE(i) == _H)
            type = ROOK;
        else if (FILE(i) == _B || FILE(i) == _G)
            type = KNIGHT;
        else if (FILE(i) == _C || FILE(i) == _F)
            type = BISHOP;
        else if (FILE(i) == _D)
            type = QUEEN;
        else
            type = KING;

        if (color == WHITE)
        {
            pos.board[i] = PIECE(WHITE, type, wpi);
            pos.pieces[WHITE][wpi] = i;
            ++wpi;
        }
        else
        {
            pos.board[i] = PIECE(BLACK, type, bpi);
            pos.pieces[BLACK][bpi] = i;
            ++bpi;
        }
        pos.hash ^= zobrist_pieces[color][type][i];
    }

    pos.side_to_move = WHITE;
    pos.en_passant = NOSQUARE;
    pos.castle = (WK_CASTLE | WQ_CASTLE | BK_CASTLE | BQ_CASTLE);
    pos.king_square[WHITE] = _E1;
    pos.king_square[BLACK] = _E8;
    pos.has_castled[WHITE] = 0;
    pos.has_castled[BLACK] = 0;
    pos.nmove = 1;
    pos.half_moves = 0;

    pos.hash ^= zobrist_castle[pos.castle];

    his->en_passant = pos.en_passant;
    his->castle = pos.castle;
    his->hash = pos.hash;
    his->pawn_hash = pos.pawn_hash;

    ply = 0;
}

/*========================================================================*//**
 * \brief Load a legal chess position
 * \todo check legal castling rights && e.p. square
 * \return 0 if the position has been loaded, an error code otherwise
 *//*=========================================================================*/
int set_fen(const char* fen)
{
	position_t newpos;
    newpos.side_to_move = WHITE;
    newpos.en_passant = NOSQUARE;
    newpos.castle = 0;
	newpos.king_square[WHITE] = NOSQUARE;
	newpos.king_square[BLACK] = NOSQUARE;
	newpos.has_castled[WHITE] = 0;
	newpos.has_castled[BLACK] = 0;
    newpos.nmove = 1;
    newpos.half_moves = 0;
	newpos.hash = 0;
    newpos.pawn_hash = 0;

    memset(newpos.board, 0, sizeof(board_t));
	for (unsigned i = 0; i < 16; i++)
	{
		newpos.pieces[BLACK][i] = NOSQUARE;
		newpos.pieces[WHITE][i] = NOSQUARE;
	}

    skip_spaces(&fen);

    int pawn_count[2] = { 0 };
	int pi[2] = { 0 };
    int rank = _8;
    while (*fen && !isspace(*fen))
    {
        int file = _A;
        int eol = 0;

        while (*fen && !isspace(*fen) && !eol)
        {
			int sq = SQUARE(file, rank);
            int count = 1;
            int color = WHITE;
            int type;
            char c = *fen++;
            if (c >= 'a' && c <= 'z')
            {
                color = BLACK;
                c -= 'a' - 'A';
            }

            switch(c)
            {
				case 'P':
					if (rank == _1 || rank == _8)	// no pawn on 1 and 8 ranks
						return FEN_ILLEGAL_PAWN_ON_8TH;
					type = PAWN;
					pawn_count[color]++;
					goto add_piece;

                case 'N': type = KNIGHT; goto add_piece;
                case 'B': type = BISHOP; goto add_piece;
                case 'R': type = ROOK; goto add_piece;
                case 'Q': type = QUEEN; goto add_piece;

                case 'K':
					if (newpos.king_square[color] != NOSQUARE)
						return FEN_ILLEGAL_TOO_MANY_KINGS;
					newpos.king_square[color] = sq;
					type = KING;
					goto add_piece;

                case '1': case '2': case '3': case '4': case '5':
                case '6': case '7': case '8':
                    count = c - '0';
                    break;

                case '/':
				case ' ':
				case '\t':
                    if (file != _H + 1)    // not enough / too many squares
                        return FEN_INVALID_SQUARE_COUNT;
                    if (rank == _1)    // unexpected '/'
                        return FEN_INVALID_SQUARE_COUNT;
                    count = 0;
                    eol = 1;
                    rank--;
                    break;

                default:        // unexpected character
                    if (!c || isspace(c))
                        return FEN_INVALID_SQUARE_COUNT;
                    return FEN_INVALID_UNKNOWN_PIECE;
add_piece:
                newpos.board[sq] = PIECE(color, type, pi[color]);
                newpos.pieces[color][pi[color]] = sq;
                ++pi[color];
                newpos.hash ^= zobrist_pieces[color][type][sq];
                if (type == PAWN)
                    newpos.pawn_hash ^= zobrist_pieces[color][PAWN][sq];
                if (pi[color] > 16)
                    return FEN_ILLEGAL_TOO_MANY_PIECES;
                break;
            }

            file += count;
            if (file > _H + 1)	// too many files
                return FEN_INVALID_SQUARE_COUNT;
        }
    }
    if (rank != _1)		// too many ranks
        return FEN_INVALID_SQUARE_COUNT;

	if (newpos.king_square[WHITE] == NOSQUARE)
		return FEN_ILLEGAL_MISSING_WHITE_KING;
	if (newpos.king_square[BLACK] == NOSQUARE)
		return FEN_ILLEGAL_MISSING_BLACK_KING;

	if (pawn_count[WHITE] > 8 || pawn_count[BLACK] > 8)
        return FEN_ILLEGAL_TOO_MANY_PAWNS;

    if (!skip_spaces(&fen))
        return FEN_INVALID_MISSING_SIDE_TO_MOVE;

    if (*fen == 'w')
        newpos.side_to_move = WHITE;
    else if (*fen == 'b')
    {
        newpos.side_to_move = BLACK;
        newpos.hash ^= zobrist_black_to_move;
    }
    else
        return FEN_INVALID_MISSING_SIDE_TO_MOVE;

    fen++;

    if (!skip_spaces(&fen))
        return FEN_INVALID_MISSING_CASTLING_RIGHTS;

    if (*fen != '-')
    {
        while (*fen == 'K' || *fen == 'Q' || *fen == 'k' || *fen == 'q')
        {
            if (*fen == 'K')
                newpos.castle |= WK_CASTLE;
            else if (*fen == 'Q')
                newpos.castle |= WQ_CASTLE;
            else if (*fen == 'k')
                newpos.castle |= BK_CASTLE;
            else if (*fen ==  'q')
                newpos.castle |= BQ_CASTLE;
            else
                return FEN_INVALID_MISSING_CASTLING_RIGHTS;

            fen++;
        }
        newpos.hash ^= zobrist_castle[newpos.castle];
    }
    else
        fen++;

    if (!skip_spaces(&fen))
        return FEN_INVALID_MISSING_EN_PASSANT;

    if (*fen != '-')
    {
        int f, r;
        if (*fen >= 'a' && *fen <= 'h')
            f = *fen - 'a';
        else
            return FEN_INVALID_MISSING_EN_PASSANT;
        fen++;
        if (*fen >= '1' && *fen <= '8')
            r = *fen - '1';
        else
            return FEN_INVALID_MISSING_EN_PASSANT;
        newpos.en_passant = SQUARE(f, r);
        newpos.hash ^= zobrist_ep_file[f];
        fen++;
    }
    else
        fen++;

    skip_spaces(&fen);

    if (*fen >= '0' && *fen <= '9')	// optional half move count
    {
        newpos.half_moves = read_num(&fen);

		skip_spaces(&fen);
		if (*fen >= '0' && *fen <= '9')
            newpos.nmove = read_num(&fen);
    }

    position_t backup;
    memcpy(&backup, &pos, sizeof(position_t));
    memcpy(&pos, &newpos, sizeof(position_t));

    if (illegal())
    {
        memcpy(&pos, &backup, sizeof(position_t));
        return FEN_ILLEGAL_IN_CHECK;
    }

    his = history_entries;
    his->en_passant = pos.en_passant;
    his->castle = pos.castle;
    his->capture = 0;
    his->hash = pos.hash;
    his->pawn_hash = pos.pawn_hash;

    ply = 0;

    return NO_ERROR;
}

/*========================================================================*//**
 * \brief Play a move regardless of whether it's legal or not but still
 * update the side to move, the castling rights, the e.p. square, etc.
 * \todo update hash when completing castling with rook move
 *//*=========================================================================*/
void make_move(move_t move)
{
    int last_en_passant = pos.en_passant;
    int type = TYPE(pos.board[move.b.from]);
    int rankF = RANK(move.b.from);
    int rankT = RANK(move.b.to);

    his++;
    his->capture = pos.board[move.b.to];
    if (his->capture)
    {
        pos.pieces[pos.side_to_move ^ BLACK][INDEX(his->capture)] = NOSQUARE;
        pos.hash ^= zobrist_pieces
                        [pos.side_to_move ^ BLACK]
                        [TYPE(his->capture)]
                        [INDEX(his->capture)];

        if (TYPE(his->capture) == PAWN)
        {
            pos.pawn_hash ^= zobrist_pieces
                                [pos.side_to_move ^ BLACK]
                                [PAWN]
                                [INDEX(his->capture)];
        }
    }

    pos.half_moves++;

    pos.en_passant = NOSQUARE;
    pos.hash ^= zobrist_ep_file[FILE(last_en_passant)]; // clear en passant

    pos.hash ^= zobrist_castle[pos.castle]; // clear castling rights

    if (type == PAWN)
    {
        pos.half_moves = 0;

        if (rankF == _2 && rankT == _4)
        {
            pos.en_passant = move.b.from + UP;
            pos.hash ^= zobrist_ep_file[FILE(move.b.from)];
        }
        else if (rankF == _7 && rankT == _5)
        {
            pos.en_passant = move.b.from + DOWN;
            pos.hash ^= zobrist_ep_file[FILE(move.b.from)];
        }
        else if (move.b.to == last_en_passant)
        {
            int cap;
            if (rankF == _5)
            {
                cap = pos.board[last_en_passant + DOWN];
                his->capture = cap;
                pos.pieces[BLACK][INDEX(cap)] = NOSQUARE;
                pos.board[last_en_passant + DOWN] = 0;
                pos.hash ^= zobrist_pieces[BLACK][PAWN][last_en_passant + DOWN];
                pos.pawn_hash ^= zobrist_pieces[BLACK][PAWN][last_en_passant + DOWN];
            }
            else
            {
                cap = pos.board[last_en_passant + UP];
                his->capture = cap;
                pos.pieces[WHITE][INDEX(cap)] = NOSQUARE;
                pos.board[last_en_passant + UP] = 0;
                pos.hash ^= zobrist_pieces[WHITE][PAWN][last_en_passant + UP];
                pos.pawn_hash ^= zobrist_pieces[WHITE][PAWN][last_en_passant + UP];
            }
        }

        if (rankT != _8 && rankT != _1)
            pos.pawn_hash ^= zobrist_pieces[pos.side_to_move][type][move.b.to];
        pos.pawn_hash ^= zobrist_pieces[pos.side_to_move][type][move.b.from];
    }
    else if (type == KING)
    {
        pos.king_square[pos.side_to_move] = move.b.to;
        if (move.b.from == _E1)
        {
            if (move.b.to == _G1)
            {
                pos.pieces[WHITE][INDEX(pos.board[_H1])] = _F1;
                pos.board[_F1] = pos.board[_H1];
                pos.board[_H1] = 0;
                pos.has_castled[WHITE] = WK_CASTLE;
                pos.hash ^= zobrist_pieces[WHITE][ROOK][_H1];
                pos.hash ^= zobrist_pieces[WHITE][ROOK][_F1];
            }
            else if (move.b.to == _C1)
            {
                pos.pieces[WHITE][INDEX(pos.board[_A1])] = _D1;
                pos.board[_D1] = pos.board[_A1];
                pos.board[_A1] = 0;
                pos.has_castled[WHITE] = WQ_CASTLE;
                pos.hash ^= zobrist_pieces[WHITE][ROOK][_A1];
                pos.hash ^= zobrist_pieces[WHITE][ROOK][_D1];
            }

            pos.castle &= ~(WK_CASTLE);
            pos.castle &= ~(WQ_CASTLE);
        }
        else if (move.b.from == _E8)
        {
            if (move.b.to == _G8)
            {
                pos.pieces[BLACK][INDEX(pos.board[_H8])] = _F8;
                pos.board[_F8] = pos.board[_H8];
                pos.board[_H8] = 0;
                pos.has_castled[BLACK] = BK_CASTLE;
                pos.hash ^= zobrist_pieces[BLACK][ROOK][_H8];
                pos.hash ^= zobrist_pieces[BLACK][ROOK][_F8];
            }
            else if (move.b.to == _C8)
            {
                pos.pieces[BLACK][INDEX(pos.board[_A8])] = _D8;
                pos.board[_D8] = pos.board[_A8];
                pos.board[_A8] = 0;
                pos.has_castled[BLACK] = BQ_CASTLE;
                pos.hash ^= zobrist_pieces[BLACK][ROOK][_A8];
                pos.hash ^= zobrist_pieces[BLACK][ROOK][_D8];
            }

            pos.castle &= ~(BK_CASTLE);
            pos.castle &= ~(BQ_CASTLE);
        }
    }
    else if (type == ROOK)
    {
        if (move.b.from == _A1)
            pos.castle &= ~(WQ_CASTLE);
        else if (move.b.from == _H1)
            pos.castle &= ~(WK_CASTLE);
        else if (move.b.from == _A8)
            pos.castle &= ~(BQ_CASTLE);
        else if (move.b.from == _H8)
            pos.castle &= ~(BK_CASTLE);
    }

    if (pos.board[move.b.to] != 0)
    {
        pos.half_moves = 0;

        if (move.b.to == _A1)
            pos.castle &= ~(WQ_CASTLE);
        else if (move.b.to == _H1)
            pos.castle &= ~(WK_CASTLE);
        else if (move.b.to == _A8)
            pos.castle &= ~(BQ_CASTLE);
        else if (move.b.to == _H8)
            pos.castle &= ~(BK_CASTLE);
    }

    pos.board[move.b.to] = pos.board[move.b.from];
    if (move.b.promotion)
    {
        pos.board[move.b.to] &= ~(0x0E);
        pos.board[move.b.to] |= (move.b.promotion << 1);
        pos.hash ^= zobrist_pieces[pos.side_to_move][move.b.promotion][move.b.to];
    }
    else
        pos.hash ^= zobrist_pieces[pos.side_to_move][type][move.b.to];

    pos.hash ^= zobrist_pieces[pos.side_to_move][type][move.b.from];
    pos.board[move.b.from] = 0;

    if (pos.side_to_move == WHITE)
    {
        pos.pieces[WHITE][INDEX(pos.board[move.b.to])] = move.b.to;
        pos.side_to_move = BLACK;
    }
    else
    {
        pos.pieces[BLACK][INDEX(pos.board[move.b.to])] = move.b.to;
        pos.side_to_move = WHITE;
        pos.nmove++;
    }

    pos.hash ^= zobrist_black_to_move;
    pos.hash ^= zobrist_castle[pos.castle];

    his->half_moves = pos.half_moves;
    his->move = move;
    his->en_passant = pos.en_passant;
    his->castle = pos.castle;
    his->hash = pos.hash;
    his->pawn_hash = pos.pawn_hash;
}

/*========================================================================*//**
 * \brief Take back the last move
 *//*=========================================================================*/
void unmake_move()
{
    if (his == history_entries)
        return;

    char type = TYPE(pos.board[his->move.b.to]);

    if (type == PAWN && his->move.b.to == (his-1)->en_passant)
    {
        // Annule la prise en passant en restaurant le pion capture
        if (RANK(his->move.b.to) == _6)
        {
            pos.pieces[BLACK][INDEX(his->capture)] = his->move.b.to + DOWN;
            pos.board[his->move.b.to + DOWN] = his->capture;
            his->capture = 0;
        }
        else
        {
            pos.pieces[WHITE][INDEX(his->capture)] = his->move.b.to + UP;
            pos.board[his->move.b.to + UP] = his->capture;
            his->capture = 0;
        }
    }
    else if (type == KING)
    {
        pos.king_square[pos.side_to_move ^ BLACK] = his->move.b.from;

        // Annule le roque en restaurant la position de la tour
        if (his->move.b.from == _E1)
        {
            if (his->move.b.to == _C1)
            {
                pos.pieces[WHITE][INDEX(pos.board[_D1])] = _A1;
                pos.board[_A1] = pos.board[_D1];
                pos.board[_D1] = 0;
                pos.has_castled[WHITE] = 0;
            }
            else if (his->move.b.to == _G1)
            {
                pos.pieces[WHITE][INDEX(pos.board[_F1])] = _H1;
                pos.board[_H1] = pos.board[_F1];
                pos.board[_F1] = 0;
                pos.has_castled[WHITE] = 0;
            }
        }
        else if (his->move.b.from == _E8)
        {
            if (his->move.b.to == _C8)
            {
                pos.pieces[BLACK][INDEX(pos.board[_D8])] = _A8;
                pos.board[_A8] = pos.board[_D8];
                pos.board[_D8] = 0;
                pos.has_castled[BLACK] = 0;
            }
            else if (his->move.b.to == _G8)
            {
                pos.pieces[BLACK][INDEX(pos.board[_F8])] = _H8;
                pos.board[_H8] = pos.board[_F8];
                pos.board[_F8] = 0;
                pos.has_castled[BLACK] = 0;
            }
        }
    }

    pos.board[his->move.b.from] = pos.board[his->move.b.to];
    if (his->move.b.promotion)
    {
        pos.board[his->move.b.from] &= ~(0x0E);
        pos.board[his->move.b.from] |= (PAWN << 1);
    }

    pos.board[his->move.b.to] = his->capture;

    if (pos.side_to_move == WHITE)
    {
        pos.pieces[BLACK][INDEX(pos.board[his->move.b.from])] = his->move.b.from;
        if (his->capture)
            pos.pieces[WHITE][INDEX(his->capture)] = his->move.b.to;

        pos.side_to_move = BLACK;
        pos.nmove--;
    }
    else
    {
        pos.pieces[WHITE][INDEX(pos.board[his->move.b.from])] = his->move.b.from;
        if (his->capture)
            pos.pieces[BLACK][INDEX(his->capture)] = his->move.b.to;
        pos.side_to_move = WHITE;
    }

    his--;

    pos.en_passant = his->en_passant;
    pos.castle = his->castle;
    pos.half_moves = his->half_moves;
    pos.hash = his->hash;
    pos.pawn_hash = his->pawn_hash;
}

/*========================================================================*//**
 * \brief Return the last move played
 *//*=========================================================================*/
move_t last_move()
{
    move_t mv = {NOSQUARE, NOSQUARE, 0};
    if (his == history_entries)
        return mv;
    mv = his->move;

    return mv;
}

/*========================================================================*//**
 * \brief Tell whether the side 'side' is in check
 *//*=========================================================================*/
int in_check(int side)
{
    return attacked(pos.king_square[side], (side ^ BLACK));
}

/*========================================================================*//**
 * \brief Tell whether the position is illegal or not
 *//*=========================================================================*/
int illegal()
{
    return in_check(pos.side_to_move ^ BLACK);
}

/*========================================================================*//**
 * \brief Tell whether the first move has been played or not
 *//*=========================================================================*/
int move_played()
{
    return (his != history_entries);
}

/*========================================================================*//**
 * \brief Clear the board and history
 *//*=========================================================================*/
void clear()
{
    memset(&pos.board, 0, sizeof(board_t));
    memset(history_entries, 0, sizeof(history_entries));
    for (unsigned i = 0; i < 16; i++)
    {
        pos.pieces[WHITE][i] = NOSQUARE;
        pos.pieces[BLACK][i] = NOSQUARE;
    }
    pos.hash = 0;
    pos.pawn_hash = 0;
    his = history_entries;
}

/*========================================================================*//**
 * \brief Check for the threefold repetiton rule
 *//*=========================================================================*/
int is_repetition()
{
    int count = 0;
    uint64_t test_hash = pos.hash;
    history_t* p = history_entries;
    while (p < his)
    {
        if (p->hash == test_hash)
        {
            count++;
            if (count == 2)
                return 1;
        }
        p++;
    }

    return 0;
}
