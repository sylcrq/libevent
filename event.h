#ifndef _MY_EVENT_H_
#define _MY_EVENT_H_

#include <sys/queue.h>

#define EVENT_READ      0x1
#define EVENT_WRITE     0x2
#define EVENT_TIMEOUT   0x4

#define EVLIST_READ     EVENT_READ
#define EVLIST_WRITE    EVENT_WRITE
#define EVLIST_TIMEOUT  EVENT_TIMEOUT
#define EVLIST_ADD      0x8
#define EVLIST_INIT     0x10

#define DEFAULT_TIMEOUT 5

struct event
{
    int ev_fd;
    int ev_type;
    void* ev_arg;

    void (*ev_callback)(int, int, void*);

    int ev_flag;

    struct timeval ev_timeout;

    TAILQ_ENTRY(event) ev_read_next;
    TAILQ_ENTRY(event) ev_write_next;
    TAILQ_ENTRY(event) ev_timeout_next;
    TAILQ_ENTRY(event) ev_add_next;
};

int event_init(void);

int event_set(struct event* evp, int fd, int type, void* arg, void (*callback)(int, int, void*));
int event_add(struct event* evp, struct timeval* timeout);
int event_delete(struct event* evp);

//void event_traversal(void);

int event_dispatch(void);

#endif
