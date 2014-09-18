#ifndef _MY_EVENT_H_
#define _MY_EVENT_H_

#include <sys/queue.h>

struct event
{
    int value;

    TAILQ_ENTRY(event) ev_next;
};

int event_init(void);
int event_add(int val);
int event_delete(int val);
void event_traversal(void);
//int event_dispatch();

#endif
