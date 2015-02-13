#include <stdio.h>
#include <stdlib.h>
#include <sys/tree.h>
#include <sys/time.h>
#include <time.h>

struct event {
    struct timeval tv;
    // (type)
    RB_ENTRY(event) entries;
};

struct event_base {
    // (name, type)
    RB_HEAD(event_tree, event) timetree;
};

int compare(struct event* a, struct event* b)
{
    if(timercmp(&(a->tv), &(b->tv), <))
        return -1;
    else if(timercmp(&(a->tv), &(b->tv), >))
        return 1;

    // timeval of a == timeval of b
    if(a < b)
        return -1;
    else if(a > b)
        return 1;

    return 0;
}

// Struct timeval to printable format
// http://stackoverflow.com/questions/2408976/struct-timeval-to-printable-format
void print_timeval(struct timeval* ptv)
{
    char tmp[64] = {0};
    char buf[64] = {0};

    struct tm* nowtime = localtime(&(ptv->tv_sec));
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", nowtime);
    snprintf(buf, sizeof(buf), "%s.%06d", tmp, ptv->tv_usec);

    printf("%s", buf);
}

// Note!
// (name, type, field, cmp)
RB_PROTOTYPE(event_tree, event, entries, compare);
RB_GENERATE(event_tree, event, entries, compare);

int main()
{
    struct event_base* my_event_base = calloc(1, sizeof(struct event_base));
    RB_INIT(&(my_event_base->timetree));

    struct event* item = NULL;
    // print
    RB_FOREACH(item, event_tree, &(my_event_base->timetree)) {
        print_timeval(&(item->tv));
        printf("->\n");
    }

    struct event* a = calloc(1, sizeof(struct event));
    struct event* b = calloc(1, sizeof(struct event));
    struct event* c = calloc(1, sizeof(struct event));

    gettimeofday(&(a->tv), NULL);
    sleep(1);
    gettimeofday(&(b->tv), NULL);
    sleep(1);
    gettimeofday(&(c->tv), NULL);

    RB_INSERT(event_tree, &(my_event_base->timetree), a);
    RB_INSERT(event_tree, &(my_event_base->timetree), b);
    RB_INSERT(event_tree, &(my_event_base->timetree), c);

    // print
    RB_FOREACH(item, event_tree, &(my_event_base->timetree)) {
        print_timeval(&(item->tv));
        printf("->\n");
    }

    return 0;
}

