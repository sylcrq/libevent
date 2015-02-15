#ifndef _MY_EVENT_INTERNAL_H_
#define _MY_EVENT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

struct event_base {
    const struct eventop* evsel;

    TAILQ_HEAD(event_list, event) eventqueue;
    RB_HEAD(event_tree, event) timetree;
};

#ifdef __cplusplus
}
#endif

#endif  /* _MY_EVENT_INTERNAL_H_ */
