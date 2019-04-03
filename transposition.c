#include "transposition.h"
#include "position.h"
#include "board.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

uint64_t zobrist_pieces[2][7][128];
uint64_t zobrist_ep_file[8];
uint64_t zobrist_castle[16];
uint64_t zobrist_black_to_move;
uint64_t initial_hash;

typedef struct
{
    uint64_t hash;
    int      val;
} tt_eval_entry_t;

typedef struct
{
    uint64_t hash;
    int      val;
} tt_pawn_entry_t;


tt_eval_entry_t* tt_eval = NULL;
unsigned         tt_eval_num_entry = 0;
tt_pawn_entry_t* tt_pawn = NULL;
unsigned         tt_pawn_num_entry = 0;

static uint64_t rand64()
{
    static uint64_t seed = 1;
    seed = (seed * 2862933555777941757ULL + 3037000493ULL);
    return seed;
}

void tt_init()
{
    for (int i = 0; i < 2; i++)
    {
        for (int j = 1; j < 7; j++)
        {
            for (int k = 0; k < 128; k++)
            {
                if (OFFBOARD(j))
                {
                    k += 8;
                    if (k > _H8)
                        break;
                }
                zobrist_pieces[i][j][k] = rand64();
            }
        }
    }

    for (int i = 0; i < 8; i++)
        zobrist_ep_file[i] = random();

    for (int i = 0; i < 16; i++)
        zobrist_castle[i] = random();

    zobrist_black_to_move = random();

    tteval_set_size(0x4000000);
    ttpawn_set_size(0x2000000);
}

void tt_destroy()
{
    free(tt_eval);
    free(tt_pawn);
}

void tteval_set_size(uint32_t size)
{
    free(tt_eval);

    if (size & (size - 1))
    {
        --size;
        size |= size >> 1;
        size |= size >> 2;
        size |= size >> 4;
        size |= size >> 8;
        size |= size >> 16;
        ++size;
        size >>= 1;
    }

    if (size < sizeof(tt_eval_entry_t))
    {
        tt_eval_num_entry = 0;
        return;
    }
    tt_eval_num_entry = (size / sizeof(tt_eval_entry_t)) - 1;
    tt_eval = (tt_eval_entry_t*)malloc(size);
    memset(tt_eval, 0, size);
}

void tteval_save(int val)
{
    if (!tt_eval_num_entry)
        return;

    tt_eval_entry_t* p = tt_eval + (pos.hash & tt_eval_num_entry);
    p->hash = pos.hash;
    p->val = val;
}

int tteval_probe(int* val)
{
    if (!tt_eval_num_entry)
        return 0;

    tt_eval_entry_t* p = tt_eval + (pos.hash & tt_eval_num_entry);
    if (p->hash == pos.hash)
    {
        *val = p->val;
        return 1;
    }
    return 0;
}

void ttpawn_set_size(uint32_t size)
{
    free(tt_pawn);

    if (size & (size - 1))
    {
        --size;
        size |= size >> 1;
        size |= size >> 2;
        size |= size >> 4;
        size |= size >> 8;
        size |= size >> 16;
        ++size;
        size >>= 1;
    }

    if (size < sizeof(tt_pawn_entry_t))
    {
        tt_pawn_num_entry = 0;
        return;
    }
    tt_pawn_num_entry = (size / sizeof(tt_pawn_entry_t)) - 1;
    tt_pawn = (tt_pawn_entry_t*)malloc(size);
    memset(tt_pawn, 0, size);
}

void ttpawn_save(int val)
{
    if (!tt_pawn_num_entry)
        return;

    tt_pawn_entry_t* p = tt_pawn + (pos.pawn_hash & tt_pawn_num_entry);
    p->hash = pos.pawn_hash;
    p->val = val;
}

int ttpawn_probe(int* val)
{
    if (!tt_pawn_num_entry)
        return 0;

    tt_pawn_entry_t* p = tt_pawn + (pos.pawn_hash & tt_pawn_num_entry);
    if (p->hash == pos.pawn_hash)
    {
        *val = p->val;
        return 1;
    }
    return 0;
}

