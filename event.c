#include <stdlib.h>

#include "event.h"

void event_subscribe(struct event* p_event, event_callback p_callback, void* p_user_ptr) {
    ++p_event->num_subscribers;
    p_event->p_subscribers = realloc(p_event->p_subscribers, p_event->num_subscribers * sizeof(*p_event->p_subscribers));
    
    struct event_subscriber* p_subscriber = &p_event->p_subscribers[p_event->num_subscribers - 1];
    *p_subscriber = (struct event_subscriber){
        .p_callback = p_callback,
        .p_user_ptr = p_user_ptr
    };
}

void event_unsubscribe(struct event* p_event, event_callback p_callback) {
    for (size_t i = 0; i < p_event->num_subscribers; ++i) {
        if (p_event->p_subscribers[i].p_callback != p_callback) {
            continue;
        }

        --p_event->num_subscribers;

        if (p_event->num_subscribers > 0 && i < p_event->num_subscribers) {
            struct event_subscriber* p_dst = &p_event->p_subscribers[i];
            struct event_subscriber* p_src = &p_event->p_subscribers[p_event->num_subscribers];
            *p_dst = *p_src;
        }

        p_event->p_subscribers = realloc(p_event->p_subscribers, p_event->num_subscribers * sizeof(*p_event->p_subscribers));

        break;
    }
}

void event_broadcast(const struct event* p_event, const void* p_event_data) {
    for (size_t i = 0; i < p_event->num_subscribers; ++i) {
        const struct event_subscriber* p_subscriber = &p_event->p_subscribers[i];
        p_subscriber->p_callback(p_event_data, p_subscriber->p_user_ptr);
    }
}

void event_free(struct event* p_event) {
    free(p_event->p_subscribers);
    *p_event = (struct event) {0};
}