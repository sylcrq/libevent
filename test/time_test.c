#include <stdio.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/time.h>
#include <time.h>

#include <event.h>

time_t g_lasttime = 0;

void timeout_cb(int fd, int events, void* arg)
{
    struct event* ev = arg;
    time_t currtime = time(NULL);

    printf("timeout_cb called at %d, %d\n", currtime, currtime-g_lasttime);

    struct timeval tv;
    timerclear(&tv);
    tv.tv_sec = 2;

    g_lasttime = currtime;

    event_add(ev, &tv);
}

int main()
{
    struct event timeout;

    event_init();

    event_set(&timeout, -1, 0, timeout_cb, &timeout);

    struct timeval tv = {2, 0};
    event_add(&timeout, &tv);

    g_lasttime = time(NULL);

    event_dispatch();

    return 0;
}
