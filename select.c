#include <stdio.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/time.h>

#include "event.h"

void* select_init(void);
int select_add(void* arg, struct event* ev);
int select_del(void* arg, struct event* ev);

const struct eventop selectops = {
    "select",
    select_init,
    select_add,
    select_del
};

void* select_init(void)
{
    return NULL;
}

int select_add(void* arg, struct event* ev)
{
    return 0;
}

int select_del(void* arg, struct event* ev)
{
    return 0;
}
