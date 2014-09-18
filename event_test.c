#include <stdio.h>

#include "event.h"

int main()
{
    event_init();

    event_add(3);
    event_add(5);
    event_add(7);
    event_add(4);
    event_add(7);

    event_traversal();

    event_delete(10);
    event_delete(7);

    event_traversal();

    return 0;
}
