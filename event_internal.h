#ifndef _MY_EVENT_INTERNAL_H_
#define _MY_EVENT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

TAILQ_HEAD(event_list, event);

struct event_base {
    const struct eventop* evsel;
    void* evbase;

    int event_count;
    int event_count_active;

    struct event_list* activequeues;
    int nactivequeues;

    struct event_list eventqueue;

    RB_HEAD(event_tree, event) timetree;
};

#ifdef __cplusplus
}
#endif

#endif  /* _MY_EVENT_INTERNAL_H_ */
