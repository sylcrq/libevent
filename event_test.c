#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "event.h"

void read_callback()
{
    printf("read\n");
}

void write_callback()
{
    printf("write\n");
}

int main()
{

    if(mkfifo("fifo123", 0600) < 0)
    {
        fprintf(stderr, "mkfifo error\n");
        return -1;
    }

    int fd = open("fifo123", O_RDWR | O_NONBLOCK, 0);
    if(fd < 0)
    {
        printf("open error\n");
        return -1;
    }

    event_init();

    struct event fifo_event;
    memset(&fifo_event, 0, sizeof(struct event));
    //event_add(fd, 0, read_callback);
    //event_add(fd, 1, write_callback);
    event_set(&fifo_event, fd, EVENT_READ, read_callback);

    event_add(&fifo_event);
#if 0
    event_add(3);
    event_add(5);
    event_add(7);
    event_add(4);
    event_add(7);

    event_traversal();

    event_delete(10);
    event_delete(7);
#endif
    //event_traversal();

    event_dispatch();

    return 0;
}
