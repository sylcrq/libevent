#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <time.h>

#include "event.h"
#include "event_internal.h"

struct event_base* g_current_base = NULL;

extern const struct eventop g_selectops;

const struct eventop* g_eventops[] = {
    &g_selectops,
    NULL
};

/* prototypes */
void event_queue_insert(struct event_base* base, struct event* ev, int queue);
void event_queue_remove(struct event_base* base, struct event* ev, int queue);

void event_process_active(struct event_base* base);

int timeout_next(struct event_base* base, struct timeval* tv);
void timeout_process(struct event_base* base);

// 将struct timeval转换为字符串
char g_timebuf[64] = {0};
char* timeval_to_str(struct timeval* ptv)
{
    char tmp[64] = {0};
    memset(g_timebuf, 0, sizeof(g_timebuf));

    struct tm* nowtime = localtime(&(ptv->tv_sec));
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", nowtime);
    snprintf(g_timebuf, sizeof(g_timebuf), "%s.%06d", tmp, ptv->tv_usec);

    return g_timebuf;
}

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

// 初始化一个全局struct event_base
void* event_init(void)
{
    printf("event_init\n");

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

    g_current_base->event_count = 0;
    g_current_base->event_count_active = 0;

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

    // 初始化默认优先级队列个数为1
    event_base_priority_init(g_current_base, 1);

    return g_current_base;
}

void event_set(struct event* ev, int fd, int events, void (*callback)(int, int, void*))
{
    //if(!ev) return;

    ev->ev_base = g_current_base;

    ev->ev_fd = fd;
    ev->ev_events = events;
    ev->ev_callback = callback;
    ev->ev_flags = EVLIST_INIT;

    ev->ev_arg = ev;
    ev->ev_res = 0;
    // event默认优先级
    ev->ev_pri = g_current_base->nactivequeues/2;
}

int event_add(struct event* ev, struct timeval* tv)
{
    //if(!ev) return -1;
    printf("event_add: ev=%p, tv=%s\n", ev, 
           (tv==NULL) ? "NULL" : timeval_to_str(tv));

    struct event_base* base = ev->ev_base;
    const struct eventop* evsel = base->evsel;
    void* evbase = base->evbase;

    assert(!(ev->ev_flags & ~EVLIST_ALL));

    if(tv != NULL)
    {
        struct timeval now;

        if(ev->ev_flags & EVLIST_TIMEOUT)
            event_queue_remove(base, ev, EVLIST_TIMEOUT);

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
    printf("event_dispatch\n");

    struct event_base* base = g_current_base;
    const struct eventop* evsel = base->evsel;
    void* evbase = base->evbase;

    struct timeval tv;

    if(evsel->recalc(base, evbase, 0) < 0)
        return -1;

    while(1)
    {
        //gettimeofday(&tv, NULL);
        if(base->event_count_active == 0)
            timeout_next(base, &tv);
        else
            timerclear(&tv);

        int ret = evsel->dispatch(base, evbase, &tv);

        if(ret < 0)
            return -1;

        timeout_process(base);

        if(base->event_count_active)
        {
            event_process_active(base);
        }

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

    printf("event_queue_insert: ev=%p, list=0x%.2x\n", ev, queue);

    base->event_count++;

    ev->ev_flags |= queue;

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
        base->event_count_active++;
        TAILQ_INSERT_TAIL(&(base->activequeues[ev->ev_pri]), ev, ev_active_next);
        break;
    default:
        printf("unknown queue\n");
        exit(1);
    }

    printf("event_queue_insert: event_count=%d, event_count_active=%d\n", 
           base->event_count, base->event_count_active);
}

void event_queue_remove(struct event_base* base, struct event* ev, int queue)
{
    if(!(ev->ev_flags & queue))
    {
        printf("not on queue\n");
        exit(1);
    }

    printf("event_queue_remove: ev=%p, list=0x%.2x\n", ev, queue);

    base->event_count--;

    ev->ev_flags &= ~queue;

    switch(queue)
    {
    case EVLIST_TIMEOUT:
        RB_REMOVE(event_tree, &(base->timetree), ev);
        break;
    case EVLIST_INSERTED:
        TAILQ_REMOVE(&(base->eventqueue), ev, ev_next);
        break;
    case EVLIST_ACTIVE:
        base->event_count_active--;
        TAILQ_REMOVE(&(base->activequeues[ev->ev_pri]), ev, ev_active_next);
        break;
    default:
        printf("unknown queue\n");
        exit(1);
    }

    printf("event_queue_remove: event_count=%d, event_count_active=%d\n", 
           base->event_count, base->event_count_active);
}

int event_base_priority_init(struct event_base* base, int npriorities)
{
    printf("event_base_priority_init: %d\n", npriorities);

    if(base->event_count_active)
        return -1;

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
    printf("event_del: ev=%p\n", ev);

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
    printf("event_active: ev=%p\n", ev);

    if(ev->ev_flags & EVLIST_ACTIVE)
        return;

    event_queue_insert(ev->ev_base, ev, EVLIST_ACTIVE);
}

int timeout_next(struct event_base* base, struct timeval* tv)
{
    struct timeval default_timeout = {5, 0};  // 5s

    struct event* ev = RB_MIN(event_tree, &(base->timetree));
    if(ev == NULL)
    {
        *tv = default_timeout;
        return 0;
    }

    struct timeval now;
    gettimeofday(&now, NULL);

    if(timercmp(&(ev->ev_timeout), &now, <))
    {
        timerclear(tv);
        return 0;
    }

    timersub(&(ev->ev_timeout), &now, tv);

    return 0;
}

void timeout_process(struct event_base* base)
{
    struct event* ev = NULL;
    struct event* next = NULL;

    struct timeval now;
    gettimeofday(&now, NULL);

    for(ev=RB_MIN(event_tree, &(base->timetree)); ev != NULL; ev=next)
    {
        if(timercmp(&(ev->ev_timeout), &now, >))
            break;

        next = RB_NEXT(event_tree, &(base->timetree), ev);

        event_del(ev);
        event_active(ev);
    }
}

void event_process_active(struct event_base* base)
{
    struct event* ev = NULL;
    struct event_list* activeq = NULL;

    if(base->event_count_active == 0)
        return;

    int i = 0;
    for(i=0; i<base->nactivequeues; i++)
    {
        if(TAILQ_FIRST(&(base->activequeues[i])) != NULL)
        {
            activeq = &(base->activequeues[i]);
            break;
        }
    }

    for(ev=TAILQ_FIRST(activeq); ev != NULL; ev=TAILQ_FIRST(activeq))
    {
        event_queue_remove(base, ev, EVLIST_ACTIVE);

        (*ev->ev_callback)(ev->ev_fd, ev->ev_res, ev->ev_arg);
    }
}

