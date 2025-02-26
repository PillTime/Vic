#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

[[noreturn]] void vicDie(char const *const msg)
{
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);
    exit(1);
}
