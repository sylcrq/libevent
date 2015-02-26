#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/tree.h>

#include "event.h"
#include "event_internal.h"

struct event_base* g_current_base = NULL;

extern const struct eventop g_selectops;

const struct eventop* g_eventops[] = {
    &g_selectops,
    NULL
};

void event_queue_insert(struct event_base* base, struct event* ev, int queue);
void event_queue_remove(struct event_base* base, struct event* ev, int queue);

int compare(struct event* a, struct event* b)
{
    if(timercmp(&(a->ev_timeout), &(b->ev_timeout), <))
        return -1;
    else if(timercmp(&(a->ev_timeout), &(b->ev_timeout), >))
        return 1;

    if(a < b)
        return -1;
    else if(a > b)
        return 1;

    return 0;
}

RB_PROTOTYPE(event_tree, event, ev_timeout_node, compare);
RB_GENERATE(event_tree, event, ev_timeout_node, compare);

// 初始化
void* event_init(void)
{
    g_current_base = calloc(1, sizeof(struct event_base));
    if(!g_current_base)
    {
        printf("calloc failed\n");
        exit(1);
    }

    TAILQ_INIT(&(g_current_base->eventqueue));
    RB_INIT(&(g_current_base->timetree));

    g_current_base->evsel = NULL;
    g_current_base->evbase = NULL;

    int i = 0;
    for(i=0; g_eventops[i] && !(g_current_base->evbase); i++)
    {
        g_current_base->evsel = g_eventops[i];
        g_current_base->evbase = g_current_base->evsel->init();
    }

    if(g_current_base->evbase == NULL)
    {
        printf("no event mechanism available\n");
        exit(1);
    }

    event_base_priority_init(g_current_base, 1);

    return g_current_base;
}

void event_set(struct event* ev, int fd, int events, void (*callback)(void* arg))
{
    if(!ev) return;

    ev->ev_base = g_current_base;

    ev->ev_fd = fd;
    ev->ev_events = events;
    ev->ev_callback = callback;
    ev->ev_flags = EVLIST_INIT;

    ev->ev_pri = g_current_base->nactivequeues/2;
}

int event_add(struct event* ev, struct timeval* tv)
{
    if(!ev) return -1;

    struct event_base* base = ev->ev_base;
    const struct eventop* evsel = base->evsel;
    void* evbase = base->evbase;

    assert(!(ev->ev_flags & ~EVLIST_ALL));

    if(tv != NULL)
    {
        struct timeval now;

        if(ev->ev_flags & EVLIST_TIMEOUT)
            event_queue_remove(base, ev, EVLIST_TIMEOUT);

        // TODO:

        gettimeofday(&now, NULL);
        timeradd(tv, &now, &(ev->ev_timeout));

        event_queue_insert(base, ev, EVLIST_TIMEOUT);
    }

    if((ev->ev_events & (EVENT_READ|EVENT_WRITE)) &&
       !(ev->ev_flags & (EVLIST_INSERTED|EVLIST_ACTIVE)))
    {
        event_queue_insert(base, ev, EVLIST_INSERTED);

        return evsel->add(evbase, ev);
    }

    return 0;
}

int event_dispatch(void)
{
    struct event_base* base = g_current_base;
    const struct eventop* evsel = base->evsel;
    void* evbase = base->evbase;

    struct timeval tv;

    if(evsel->recalc(base, evbase, 0) < 0)
        return -1;

    while(1)
    {
        // TODO:

        int ret = evsel->dispatch(base, evbase, &tv);

        if(ret < 0)
            return -1;

        // TODO:

        if(evsel->recalc(base, evbase, 0) < 0)
            return -1;
    }

    return 0;
}

void event_queue_insert(struct event_base* base, struct event* ev, int queue)
{
    if(ev->ev_flags & queue)
    {
        if(queue & EVLIST_ACTIVE)
            return;

        printf("already on queue\n");
        exit(1);
    }

    switch(queue)
    {
    case EVLIST_TIMEOUT:
    {
        struct event* tmp = RB_INSERT(event_tree, &(base->timetree), ev);
        assert(tmp == NULL);
        break;
    }
    case EVLIST_INSERTED:
        TAILQ_INSERT_TAIL(&(base->eventqueue), ev, ev_next);
        break;
    case EVLIST_ACTIVE:
        TAILQ_INSERT_TAIL(&(base->activequeues[ev->ev_pri]), ev, ev_active_next);
        break;
    default:
        printf("unknown queue\n");
        exit(1);
    }

    ev->ev_flags |= queue;
}

void event_queue_remove(struct event_base* base, struct event* ev, int queue)
{
    if(!(ev->ev_flags & queue))
    {
        printf("not on queue\n");
        exit(1);
    }

    switch(queue)
    {
    case EVLIST_TIMEOUT:
        RB_REMOVE(event_tree, &(base->timetree), ev);
        break;
    case EVLIST_INSERTED:
        TAILQ_REMOVE(&(base->eventqueue), ev, ev_next);
        break;
    case EVLIST_ACTIVE:
        TAILQ_REMOVE(&(base->activequeues[ev->ev_pri]), ev, ev_active_next);
        break;
    default:
        printf("unknown queue\n");
        exit(1);
    }

    ev->ev_flags &= ~queue;
}

int event_base_priority_init(struct event_base* base, int npriorities)
{
    if(base->activequeues)
        free(base->activequeues);

    base->nactivequeues = npriorities;
    base->activequeues = (struct event_list*)calloc(npriorities, sizeof(struct event_list));
    if(base->activequeues == NULL)
    {
        printf("calloc failed");
        exit(1);
    }

    int i = 0;
    for(i=0; i<npriorities; i++)
    {
        TAILQ_INIT(&(base->activequeues[i]));
    }

    return 0;
}

int event_del(struct event* ev)
{
    struct event_base* base = ev->ev_base;
    const struct eventop* evsel = base->evsel;
    void* evbase = base->evbase;

    assert(!(ev->ev_flags & ~EVLIST_ALL));

    if(ev->ev_flags & EVLIST_TIMEOUT)
        event_queue_remove(base, ev, EVLIST_TIMEOUT);

    if(ev->ev_flags & EVLIST_ACTIVE)
        event_queue_remove(base, ev, EVLIST_ACTIVE);

    if(ev->ev_flags & EVLIST_INSERTED)
    {
        event_queue_remove(base, ev, EVLIST_INSERTED);
        return evsel->del(evbase, ev);
    }
    return 0;
}

void event_active(struct event* ev)
{
    if(ev->ev_flags & EVLIST_ACTIVE)
        return;

    event_queue_insert(ev->ev_base, ev, EVLIST_ACTIVE);
}

