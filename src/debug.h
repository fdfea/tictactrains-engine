#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdbool.h>
#include <stdio.h>

#define DEBUG_RANDOM_SEED   20

typedef enum DebugLevel
{
    DEBUG_LEVEL_NONE    = 1,
    DEBUG_LEVEL_INFO    = 2,
    DEBUG_LEVEL_WARN    = 3,
    DEBUG_LEVEL_ERROR   = 4,
} 
eDebugLevel;

#ifdef DEBUG
#define dbg_printf(Level, Format, ...)                  \
    do {                                                \
        debug_printf(Level, Format, ## __VA_ARGS__);    \
    } while (false)
#elif DEBUGV
#define dbg_printf(Level, Format, ...)                      \
    do {                                                    \
        fprintf(stderr, "%s(%d): ", __func__, __LINE__);    \
        debug_printf(Level, Format, ## __VA_ARGS__);        \
    } while (false)
#else
#define dbg_printf(Level, Format, ...)  \
    do {                                \
        { }                             \
    } while (false)
#endif

void debug_printf(eDebugLevel Level, const char *Format, ...);

#endif
