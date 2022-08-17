#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "util.h"

void *emalloc(size_t Size)
{
    void *pMemory = malloc(Size);
    if (pMemory IS NULL AND Size != 0)
    {
        dbg_printf(DEBUG_LEVEL_ERROR, "No memory available");
        exit(-ENOMEM);
    }

    return pMemory;
}

void *erealloc(void *pMemory, size_t Size)
{
    pMemory = realloc(pMemory, Size);
    if (pMemory IS NULL AND Size != 0)
    {
        dbg_printf(DEBUG_LEVEL_ERROR, "No memory available");
        exit(-ENOMEM);
    }

    return pMemory;
}
