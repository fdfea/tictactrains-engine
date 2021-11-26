#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_RANDOM_SEED   20

#define DEBUG_INFO          1
#define DEBUG_WARN          2
#define DEBUG_ERROR         3

#define DEBUG_LABEL_SIZE    10

#define DEBUG_LABEL_INFO    "[INFO] "
#define DEBUG_LABEL_WARN    "[WARN] "
#define DEBUG_LABEL_ERROR   "[ERROR] "
#define DEBUG_LABEL_NONE    ""

#define DEBUG_MATCH_LABEL(pLabel, Level)                                                        \
    do {                                                                                        \
        switch (Level) {                                                                        \
            case DEBUG_INFO: { strncpy(pLabel, DEBUG_LABEL_INFO, DEBUG_LABEL_SIZE); break; }    \
            case DEBUG_WARN: { strncpy(pLabel, DEBUG_LABEL_WARN, DEBUG_LABEL_SIZE); break; }    \
            case DEBUG_ERROR: { strncpy(pLabel, DEBUG_LABEL_ERROR, DEBUG_LABEL_SIZE); break; }  \
            default: { strncpy(pLabel, DEBUG_LABEL_NONE, DEBUG_LABEL_SIZE); break; }            \
        }                                                                                       \
    } while (false)

#ifdef DEBUGDEV
#define dbg_printf(Level, Fmt, ...)                                                     \
    do {                                                                                \
        char Label[DEBUG_LABEL_SIZE];                                                   \
        DEBUG_MATCH_LABEL(Label, Level);                                                \
        fprintf(stderr, "%s(%d): %s" Fmt, __func__, __LINE__, Label, ## __VA_ARGS__);   \
    } while (false)
#elif DEBUG
#define dbg_printf(Level, Fmt, ...)                         \
    do {                                                    \
        char Label[DEBUG_LABEL_SIZE];                       \
        DEBUG_MATCH_LABEL(Label, Level);                    \
        fprintf(stderr, "%s" Fmt, Label, ## __VA_ARGS__);   \
    } while (false)
#else
#define dbg_printf(Level, Fmt, ...) \
    do {                            \
        { }                         \
    } while (false)
#endif

#endif
