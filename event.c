#include <stdio.h>
#include <stdlib.h>

#include "event.h"

TAILQ_HEAD(event_list, event) queue;


int event_init(void)
{
    TAILQ_INIT(&queue);

    return 0;
}

int event_add(int val)
{
    struct event* evp = (struct event*)malloc(sizeof(struct event));
    if(!evp)
    {
        printf("malloc failed\n");
        return -1;
    }

    evp->value = val;

    TAILQ_INSERT_TAIL(&queue, evp, ev_next);

    return 0;
}

int event_delete(int val)
{
    struct event* evp = NULL;

    for(evp = queue.tqh_first; evp != NULL; evp = evp->ev_next.tqe_next)
    {
        if(val == evp->value)
        {
            break;
        }
    }

    if(evp != NULL)
    {
        TAILQ_REMOVE(&queue, evp, ev_next);
    }

    return 0;
}

void event_traversal(void)
{
    struct event* evp = NULL;

    for(evp = queue.tqh_first; evp != NULL; evp = evp->ev_next.tqe_next)
    {
        printf("%d -> ", evp->value);
    }

    printf("null\n");
}

