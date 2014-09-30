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
TAILQ_HEAD(event_tlist, event) timeout_queue;
TAILQ_HEAD(event_alist, event) add_queue;

// Global Variable
int g_max_fds = 0;
fd_set g_read_fds;
fd_set g_write_fds;

int g_event_loop = 0;

int event_init(void)
{
    fprintf(stdout, "event_init\n");

    TAILQ_INIT(&read_queue);
    TAILQ_INIT(&write_queue);
    TAILQ_INIT(&timeout_queue);
    TAILQ_INIT(&add_queue);

    return 0;
}

int event_set(struct event* evp, int fd, int type, void* arg, void (*callback)(int, int, void*))
{
    if(!evp)
    {
        return -1;
    }

    evp->ev_fd = fd;
    evp->ev_type = type;
    evp->ev_arg = arg;
    evp->ev_callback = callback;
    evp->ev_flag = EVLIST_INIT;

    return 0;
}

int event_add_post(struct event* evp)
{
    if(!evp)
    {
        return -1;
    }

    fprintf(stdout, "event_add_post: %p[%d, %d]\n", evp, evp->ev_fd, evp->ev_type);

    if( (evp->ev_type & EVENT_READ) && !(evp->ev_flag & EVLIST_READ) )
    {
        TAILQ_INSERT_TAIL(&read_queue, evp, ev_read_next);
        evp->ev_flag |= EVLIST_READ;
    }

    if( (evp->ev_type & EVENT_WRITE) && !(evp->ev_flag & EVLIST_WRITE) )
    {
        TAILQ_INSERT_TAIL(&write_queue, evp, ev_write_next);
        evp->ev_flag |= EVLIST_WRITE;
    }

    return 0;
}

int event_add(struct event* evp, struct timeval* timeout)
{
    if(!evp)
    {
        return -1;
    }

    fprintf(stdout, "event_add: %p[%d, %d]\n", evp, evp->ev_fd, evp->ev_type);

    if(timeout != NULL)
    {
        struct timeval now;
        timerclear(&now);

        gettimeofday(&now, NULL);

        timeradd(&now, timeout, &(evp->ev_timeout));

        fprintf(stdout, "event_add: timeout=[%ld,%ld]\n", timeout->tv_sec, timeout->tv_usec);

        if(evp->ev_flag & EVLIST_TIMEOUT)
            TAILQ_REMOVE(&timeout_queue, evp, ev_timeout_next);

        struct event* tmp;
        for(tmp=TAILQ_FIRST(&timeout_queue); tmp != NULL; tmp=TAILQ_NEXT(tmp, ev_timeout_next))
        {
            if(!timercmp(&(evp->ev_timeout), &(tmp->ev_timeout), >))
                break;
        }

        if(!tmp)
            TAILQ_INSERT_TAIL(&timeout_queue, evp, ev_timeout_next);
        else
            TAILQ_INSERT_BEFORE(tmp, evp, ev_timeout_next);

        evp->ev_flag |= EVLIST_TIMEOUT;
    }

    if(g_event_loop)
    {
        if(evp->ev_flag & EVLIST_ADD)
            return 0;

        TAILQ_INSERT_TAIL(&add_queue, evp, ev_add_next);
        evp->ev_flag |= EVLIST_ADD;
    }
    else
    {
        event_add_post(evp);
    }

    return 0;
}

int event_delete(struct event* evp)
{
    if(!evp)
    {
        return -1;
    }

    fprintf(stdout, "event_delete: %p[%d, %d]\n", evp, evp->ev_fd, evp->ev_type);

    if(evp->ev_flag & EVLIST_ADD)
    {
        TAILQ_REMOVE(&add_queue, evp, ev_add_next);
        evp->ev_flag &= ~EVLIST_ADD;
    }

    if(evp->ev_flag & EVLIST_READ)
    {
        TAILQ_REMOVE(&read_queue, evp, ev_read_next);
        evp->ev_flag &= ~EVLIST_READ;
    }

    if(evp->ev_flag & EVLIST_WRITE)
    {
        TAILQ_REMOVE(&write_queue, evp, ev_write_next);
        evp->ev_flag &= ~EVLIST_WRITE;
    }

    if(evp->ev_flag & EVLIST_TIMEOUT)
    {
        TAILQ_REMOVE(&timeout_queue, evp, ev_timeout_next);
        evp->ev_flag &= ~EVLIST_TIMEOUT;
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
    struct event* evp;

    while(1)
    {
        g_max_fds = 0;
        FD_ZERO(&g_read_fds);
        FD_ZERO(&g_write_fds);

        TAILQ_FOREACH(evp, &read_queue, ev_read_next)
        {
            FD_SET(evp->ev_fd, &g_read_fds);
            g_max_fds = MAX(g_max_fds, evp->ev_fd);
        }

        TAILQ_FOREACH(evp, &write_queue, ev_write_next)
        {
            FD_SET(evp->ev_fd, &g_write_fds);
            g_max_fds = MAX(g_max_fds, evp->ev_fd);
        }

        struct timeval tv;
        struct timeval now;

        timerclear(&tv);
        timerclear(&now);

        gettimeofday(&now, NULL);

        evp = TAILQ_FIRST(&timeout_queue);

        if(!evp)
            tv.tv_sec = DEFAULT_TIMEOUT;
        else if(timercmp(&(evp->ev_timeout), &now, >))
            timersub(&(evp->ev_timeout), &now, &tv);

        int ret = select(g_max_fds + 1, &g_read_fds, &g_write_fds, NULL, &tv);

        if(ret < 0)
        {
            fprintf(stderr, "select error\n");
            return -1;
        }
        else if(ret == 0)
        {
            fprintf(stdout, "nothing happen\n");
        }
        else
        {
            struct event* next;

            g_event_loop = 1;  // Start

            for(evp=TAILQ_FIRST(&read_queue); evp != NULL; )
            {
                next = TAILQ_NEXT(evp, ev_read_next);

                if(FD_ISSET(evp->ev_fd, &g_read_fds))
                {
                    event_delete(evp);
                    evp->ev_callback(evp->ev_fd, EVENT_READ, evp->ev_arg);
                }

                evp = next;
            }

            for(evp=TAILQ_FIRST(&write_queue); evp != NULL; )
            {
                next = TAILQ_NEXT(evp, ev_write_next);

                if(FD_ISSET(evp->ev_fd, &g_write_fds))
                {
                    event_delete(evp);
                    evp->ev_callback(evp->ev_fd, EVENT_WRITE, evp->ev_arg);
                }

                evp = next;
            }

            g_event_loop = 0;  // End

            for(evp=TAILQ_FIRST(&add_queue); evp != NULL; )
            {
                next = TAILQ_NEXT(evp, ev_add_next);

                TAILQ_REMOVE(&add_queue, evp, ev_add_next);
                evp->ev_flag &= ~EVLIST_ADD;

                event_add_post(evp);

                evp = next;
            }
        }

        //struct timeval now;
        timerclear(&now);

        gettimeofday(&now, NULL);

        while((evp = TAILQ_FIRST(&timeout_queue)) != NULL)
        {
            if(timercmp(&(evp->ev_timeout), &now, >))
                break;

            event_delete(evp);
            //TAILQ_REMOVE(&timeout_queue, evp, ev_timeout_next);
            //evp->ev_flag &= ~EVLIST_TIMEOUT;

            evp->ev_callback(evp->ev_fd, EVENT_TIMEOUT, evp->ev_arg);
        }
    }

    return 0;
}

