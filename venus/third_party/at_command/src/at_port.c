#include "at_port.h"

#include <stdlib.h>

#include "ql_rtos.h"

void *at_malloc(unsigned int nbytes)
{
    return malloc(nbytes);
}

void at_free(void *ptr)
{
    free(ptr);
}

unsigned int at_get_ms(void)
{
    return (unsigned int)ql_rtos_get_systicks();
}
