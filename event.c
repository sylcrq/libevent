#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "event.h"

#define MAX(a, b)   ((a>b) ? (a) : (b))
#define MIN(a, b)   ((a<b) ? (a) : (b))

TAILQ_HEAD(event_rlist, event) read_queue;
TAILQ_HEAD(event_wlist, event) write_queue;

// Global Variable
int g_max_fds;
fd_set g_read_fds;
fd_set g_write_fds;


int event_init(void)
{
    g_max_fds = 0;
    FD_ZERO(&g_read_fds);
    FD_ZERO(&g_write_fds);

    TAILQ_INIT(&read_queue);
    TAILQ_INIT(&write_queue);

    return 0;
}

int event_set(struct event* evp, int fd, int type, void (*callback)(void))
{
    if(evp != NULL)
    {
        evp->ev_fd = fd;
        evp->ev_type = type;
        evp->ev_callback = callback;
    }

    return 0;
}

int event_add(struct event* evp)
{
    if(evp != NULL)
    {
        if(evp->ev_type & EVENT_READ)
        {
            TAILQ_INSERT_TAIL(&read_queue, evp, ev_read_next);

            FD_SET(evp->ev_fd, &g_read_fds);
            g_max_fds = MAX(g_max_fds, evp->ev_fd);
        }

        if(evp->ev_type & EVENT_WRITE)
        {
            TAILQ_INSERT_TAIL(&write_queue, evp, ev_write_next);

            FD_SET(evp->ev_fd, &g_write_fds);
            g_max_fds = MAX(g_max_fds, evp->ev_fd);
        }
    }

    return 0;
}

int event_delete(struct event* evp)
{
    if(evp != NULL)
    {
        if(evp->ev_type & EVENT_READ)
        {
            TAILQ_REMOVE(&read_queue, evp, ev_read_next);
            
            FD_CLR(evp->ev_fd, &g_read_fds);
            //TODO: Update g_max_fds
        }

        if(evp->ev_type & EVENT_WRITE)
        {
            TAILQ_REMOVE(&write_queue, evp, ev_write_next);

            FD_CLR(evp->ev_fd, &g_write_fds);
            //TODO: Update g_max_fds
        }
    }

    return 0;
}

//void event_traversal(void)
//{
//    struct event* evp = NULL;
//
//    for(evp = queue.tqh_first; evp != NULL; evp = evp->ev_next.tqe_next)
//    {
//        printf("%d -> ", evp->value);
//    }
//
//    printf("null\n");
//}

int event_dispatch(void)
{
    while(1)
    {
        int ret = select(g_max_fds + 1, &g_read_fds, &g_write_fds, NULL, NULL);

        if(ret < 0)
        {
            fprintf(stderr, "select error\n");
        }
        else if(ret == 0)
        {
            fprintf(stdout, "nothing happen\n");
        }
        else
        {
            struct event* evp;
            TAILQ_FOREACH(evp, &read_queue, ev_read_next)
            {
                if(FD_ISSET(evp->ev_fd, &g_read_fds))
                {
                    evp->ev_callback();
                }
            }

            TAILQ_FOREACH(evp, &write_queue, ev_write_next)
            {
                if(FD_ISSET(evp->ev_fd, &g_write_fds))
                {
                    evp->ev_callback();
                }
            }
        }
    }

    return 0;
}
