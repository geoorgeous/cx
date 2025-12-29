#ifndef _H__EVENT
#define _H__EVENT

#include <stdint.h>

#define EVENT_MAX_SUBSCRIBERS 16

typedef void(*event_callback)(const void* p_event_data, void* p_user_ptr);

struct event_subscriber {
    event_callback p_callback;
    void*          p_user_ptr;
};

struct event {
    struct event_subscriber* p_subscribers;
    size_t                   num_subscribers;
};

void event_subscribe(struct event* p_event, event_callback p_callback, void* p_user_ptr);
void event_unsubscribe(struct event* p_event, event_callback p_callback);
void event_broadcast(const struct event* p_event, const void* p_event_data);
void event_free(struct event* p_event);

#endif