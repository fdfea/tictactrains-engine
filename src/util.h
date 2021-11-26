#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdbool.h>
#include <stdlib.h>

#define NOT     !
#define AND     &&
#define OR      ||
#define IS      ==
#define ISNOT   !=
#define IF      
#define THEN    ?
#define ELSE    :

#define SET_IF_GREATER(Val, MaxVal)                 \
    do {                                            \
        { if (Val > MaxVal) { MaxVal = Val; } }     \
    } while (false)

#define SET_IF_GREATER_EQ(Val, MaxVal)              \
    do {                                            \
        { if (Val >= MaxVal) { MaxVal = Val; } }    \
    } while (false)

#define SET_IF_GREATER_W_EXTRA(Val, MaxVal, Extra, MaxExtra)        \
    do {                                                            \
        { if (Val > MaxVal) { MaxVal = Val; MaxExtra = Extra; } }   \
    } while (false)

#define SET_IF_GREATER_EQ_W_EXTRA(Val, MaxVal, Extra, MaxExtra)     \
    do {                                                            \
        { if (Val >= MaxVal) { MaxVal = Val; MaxExtra = Extra; } }  \
    } while (false)

#define FREE_AND_NULLIFY(Ptr)       \
    do {                            \
        { free(Ptr); Ptr = NULL; }  \
    } while (false)

#endif
