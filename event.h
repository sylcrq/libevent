#ifndef _MY_EVENT_H_
#define _MY_EVENT_H_

#include <sys/queue.h>

#define EVENT_READ  0x1
#define EVENT_WRITE 0x2

struct event
{
    int ev_fd;
    int ev_type;
    void* ev_arg;

    void (*ev_callback)(int, int, void*);

    //struct timeval ev_timeout;

    TAILQ_ENTRY(event) ev_read_next;
    TAILQ_ENTRY(event) ev_write_next;
};

int event_init(void);

int event_set(struct event* evp, int fd, int type, void* arg, void (*callback)(int, int, void*));
int event_add(struct event* evp);
int event_delete(struct event* evp);

//void event_traversal(void);

int event_dispatch(void);

#endif
