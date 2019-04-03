#include "utils.h"
#include "board.h"
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include <sys/time.h>
#include <sys/timeb.h>



unsigned int get_time_ms()
{
    #ifdef WIN32
    return GetTickCount();
    #else
    struct timeb timebuf;
    ftime(&timebuf);
    uint32_t ms = timebuf.time * 1000 + timebuf.millitm;
    return ms;
    #endif
}


/*========================================================================*//**
 * \brief Advance the the string pointer '*p' to the next non-space character
 * \return The next non space character
 *//*=========================================================================*/
int skip_spaces(char** p)
{
	while (isspace(**p))
		(*p)++;
	return **p;
}

/*========================================================================*//**
 * \brief Read a positive integer value and advance the pointer '*p' to the
 * next non-digit character
 *//*=========================================================================*/
unsigned int read_num(char** p)
{
    unsigned int val = 0;
    while (**p >= '0' && **p <= '9')
    {
        val *= 10;
        val += (**p) - '0';
        (*p)++;
    }
    return val;
}

const char* move_to_str(move_t mv)
{
    static char buf[6];
    buf[0] = FILE(mv.b.from) + 'a';
    buf[1] = RANK(mv.b.from) + '1';
    buf[2] = FILE(mv.b.to) + 'a';
    buf[3] = RANK(mv.b.to) + '1';
    buf[4] = "\0\0nbrq"[mv.b.promotion];
    buf[5] = 0;
    return buf;
}

move_t str_to_move(const char* str)
{
    const move_t err = { .b.from = NOSQUARE,
                         .b.to = NOSQUARE,
                         .b.promotion = 0,
                         .b.bits = 0 };

    if (strlen(str) < 4)
        return err;

    move_t mv = { .u = 0 };

    if (str[0] >= 'a' && str[0] <= 'h' && str[1] >= '1' && str[1] <= '8')
        mv.b.from = SQUARE(str[0] - 'a', str[1] - '1');
    else
        return err;

    if (str[2] >= 'a' && str[2] <= 'h' && str[3] >= '1' && str[3] <= '8')
        mv.b.to = SQUARE(str[2] - 'a', str[3] - '1');
    else
        return err;

    if (mv.b.from == mv.b.to)
        return err;

    if (!str[4] || isspace(str[4]))
        return mv;

    switch (str[4])
    {
        case 'n': mv.b.promotion = KNIGHT; break;
        case 'b': mv.b.promotion = BISHOP; break;
        case 'r': mv.b.promotion = ROOK; break;
        case 'q': mv.b.promotion = QUEEN; break;
        default:
            return err;
    }

    if (!str[5] || isspace(str[5]))
        return mv;

    return err;
}
