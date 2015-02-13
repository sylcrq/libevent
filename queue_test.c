#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>

struct event {
    int value;
    TAILQ_ENTRY(event) entries;
};

#if 0
TAILQ_HEAD(event_list, event);
struct event_list eventqueue;
#else
TAILQ_HEAD(, event) eventqueue;
#endif

int main()
{
    struct event* item = NULL;

    TAILQ_INIT(&eventqueue);

    // print
    TAILQ_FOREACH(item, &eventqueue, entries) {
        printf("%d->", item->value);
    }
    printf("\n");

    struct event* a = calloc(1, sizeof(struct event));
    struct event* b = calloc(1, sizeof(struct event));
    struct event* c = calloc(1, sizeof(struct event));
    a->value = 1;
    b->value = 2;
    c->value = 3;

    TAILQ_INSERT_TAIL(&eventqueue, a, entries);
    TAILQ_INSERT_TAIL(&eventqueue, b, entries);
    TAILQ_INSERT_TAIL(&eventqueue, c, entries);
    
    // print
    TAILQ_FOREACH(item, &eventqueue, entries) {
        printf("%d->", item->value);
    }
    printf("\n");

    return 0;
}

