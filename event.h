#ifndef _MY_EVENT_H_
#define _MY_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_READ    0x01
#define EVENT_WRITE   0x02
#define EVENT_TIMEOUT 0x04
#define EVENT_SIGNAL  0x08

#define EVLIST_INIT     0x01
#define EVLIST_INSERTED 0x02
#define EVLIST_TIMEOUT  0x04
#define EVLIST_SIGNAL   0x08
#define EVLIST_ACTIVE   0x10

#define EVLIST_ALL      0x1F

struct event {
    TAILQ_ENTRY(event) ev_next;
    TAILQ_ENTRY(event) ev_active_next;
    RB_ENTRY(event) ev_timeout_node;

    struct event_base* ev_base;

    int ev_fd;
    int ev_events;

    void (*ev_callback)(void* arg);

    struct timeval ev_timeout;

    int ev_flags;

    int ev_pri;
};


struct eventop {
    const char* name;
    void* (*init)(void);
    int (*add)(void*, struct event*);
    int (*del)(void*, struct event*);
    // int (*recalc)();
    // int (*dispatch)();
};

void* event_init(void);
void event_set(struct event*, int, int, void (*)(void*));
int event_add(struct event*, struct timeval*);
int event_dispatch(void);

int event_base_priority_init(struct event_base*, int);

#ifdef __cplusplus
}
#endif

#endif  /* _MY_EVENT_H_ */

