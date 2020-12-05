#ifndef UTILS_H
#define UTILS_H

#include "move.h"

unsigned int  get_time_ms();
int           skip_spaces(char** p);
unsigned int  read_num(char** p);
const char*   move_to_str(move_t mv);
move_t        str_to_move(const char* str);
void          str_iappend(char** str, unsigned int i);

#endif
