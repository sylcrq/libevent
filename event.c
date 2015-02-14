#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/tree.h>

#include "event.h"
#include "event_internal.h"

void* event_init(void)
{
    return NULL;
}

void event_set(struct event* ev, int fd, int events, void (*callback)(void* arg))
{
}

int event_add(struct event* ev, struct timeval* timeout)
{
    return 0;
}

int event_dispatch(void)
{
    return 0;
}


