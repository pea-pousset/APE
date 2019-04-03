#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <stdint.h>

extern uint64_t zobrist_pieces[2][7][128];
extern uint64_t zobrist_ep_file[8];
extern uint64_t zobrist_castle[16]; // 1 = K, 2 = Q, 3 = KQ, etc
extern uint64_t zobrist_black_to_move;

extern uint64_t initial_hash;   ///< hash of the initial position

void tt_init();
void tt_destroy();
void tteval_set_size(uint32_t size);
void tteval_save(int val);
int  tteval_probe(int* val);
void ttpawn_set_size(uint32_t size);
void ttpawn_save(int val);
int  ttpawn_probe(int* val);

#endif
