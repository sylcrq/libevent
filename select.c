#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include "event.h"
#include "event_internal.h"

#ifndef howmany
#define howmany(x, y)   (((x)+((y)-1))/(y))
#endif

void* select_init(void);
int select_add(void* arg, struct event* ev);
int select_del(void* arg, struct event* ev);
int select_recalc(struct event_base* base, void* arg, int max);
int select_dispatch(struct event_base* base, void* arg, struct timeval* tv);

const struct eventop g_selectops = {
    "select",
    select_init,
    select_add,
    select_del,
    select_recalc,
    select_dispatch
};

struct selectop {
    int event_fds;  // max fd
    int event_fdsz;  // fd_set size
    fd_set* event_readset;
    fd_set* event_writeset;
}g_sop;

void* select_init(void)
{
    memset(&g_sop, 0, sizeof(g_sop));

    return &g_sop;
}

int select_add(void* arg, struct event* ev)
{
    struct selectop* sop = arg;

    if(ev->ev_fd > sop->event_fds)
        sop->event_fds = ev->ev_fd;
    
    return 0;
}

int select_del(void* arg, struct event* ev)
{
    //struct selectop* sop = arg;

    return 0;
}

int select_recalc(struct event_base* base, void* arg, int max)
{
    struct selectop* sop = arg;

    if(max > sop->event_fds)
        sop->event_fds = max;

    if(sop->event_fds == 0)
    {
        struct event* ev = NULL;
        TAILQ_FOREACH(ev, &(base->eventqueue), ev_next)
        {
            if(ev->ev_fd > sop->event_fds)
                sop->event_fds = ev->ev_fd;
        }
    }

    int fdsz = howmany(sop->event_fds+1, NFDBITS) * sizeof(fd_mask);

    if(fdsz > sop->event_fdsz)
    {
        fd_set* readset = realloc(sop->event_readset, fdsz);
        if(readset == NULL)
        {
            printf("realloc failed\n");
            return -1;
        }

        fd_set* writeset = realloc(sop->event_writeset, fdsz);
        if(writeset == NULL)
        {
            free(readset);
            printf("realloc failed\n");
            return -1;
        }
    
        //memset(readset, 0, fdsz);
        //memset(writeset, 0, fdsz);

        sop->event_readset = readset;
        sop->event_writeset = writeset;
        sop->event_fdsz = fdsz;
    }

    return 0;
}

int select_dispatch(struct event_base* base, void* arg, struct timeval* tv)
{
    struct selectop* sop = arg;

    memset(sop->event_readset, 0, sop->event_fdsz);
    memset(sop->event_writeset, 0, sop->event_fdsz);

    struct event* ev = NULL;
    TAILQ_FOREACH(ev, &(base->eventqueue), ev_next)
    {
        if(ev->ev_events & EVENT_READ)
            FD_SET(ev->ev_fd, sop->event_readset);

        if(ev->ev_events & EVENT_WRITE)
            FD_SET(ev->ev_fd, sop->event_writeset);
    }

    int ret = select(sop->event_fds+1, sop->event_readset, sop->event_writeset, NULL, tv);

    if(ret < 0)
    {
        if(errno != EINTR)
        {
            printf("select error\n");
            return -1;
        }

        return 0;
    }

    int maxfd = 0;
    struct event* next = NULL;
    for(ev=TAILQ_FIRST(&(base->eventqueue)); ev != NULL; ev=next)
    {
        next = TAILQ_NEXT(ev, ev_next);

        int res = 0;
        if(FD_ISSET(ev->ev_fd, sop->event_readset))
            res |= EVENT_READ;

        if(FD_ISSET(ev->ev_fd, sop->event_writeset))
            res |= EVENT_WRITE;

        res &= ev->ev_events;

        if(res)
        {
            if(!(ev->ev_events & EVENT_PERSIST))
                event_del(ev);
            else if(ev->ev_fd > maxfd)
                maxfd = ev->ev_fd;

            event_active(ev, res);
        }
        else if(ev->ev_fd > maxfd)
        {
            maxfd = ev->ev_fd;
        }
    }

    sop->event_fds = maxfd;

    return 0;
}

