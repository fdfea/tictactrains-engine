#include <stdarg.h>
#include <stdio.h>

#include "debug.h"

static const char *DEBUG_LABEL_NONE     = "";
static const char *DEBUG_LABEL_INFO     = "[INFO] ";
static const char *DEBUG_LABEL_WARN     = "[WARN] ";
static const char *DEBUG_LABEL_ERROR    = "[ERROR] ";

static const char *debug_label(eDebugLevel Level);

void debug_printf(eDebugLevel Level, const char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);

    fprintf(stderr, "%s", debug_label(Level));
    vfprintf(stderr, Format, Args);
    fprintf(stderr, "\n");

    va_end(Args);
}

static const char *debug_label(eDebugLevel Level)
{
    const char *pLabel;

    switch (Level)
    {
        case DEBUG_LEVEL_INFO: pLabel = DEBUG_LABEL_INFO; break;
        case DEBUG_LEVEL_WARN: pLabel = DEBUG_LABEL_WARN; break;
        case DEBUG_LEVEL_ERROR: pLabel = DEBUG_LABEL_ERROR; break;
        default: pLabel = DEBUG_LABEL_NONE; break;
    }

    return pLabel;
}
