#include "../inc/easyapp_event.h"

static easyapp_event_t event_buffer[event_buffer_nums];
static uint32_t event_buffer_idx;

easyapp_eventqueue_t eventqueue;

easyapp_event_t* sp_easyapp_event_get(void)
{
    if(eventqueue.Head == eventqueue.Tail)
        return NULL; //FULL

     easyapp_event_t* event = eventqueue.eventqueue[eventqueue.Head];
     eventqueue.Head++;

     if(eventqueue.Head == event_queue_nums)
        eventqueue.Head = 0;

    return event;
}

easyapp_event_t* sp_easyapp_event_create(EASYAPP_RIGISTERED_EVENTS_t event_id, uint32_t event_flags, void *event_data)
{
    if(event_id <= 0 )
        return NULL;

    uint32_t count = 0;

    while(event_buffer[event_buffer_idx].event_id != 0)
    {
        event_buffer_idx++;
        if(event_buffer_idx == event_buffer_nums)
            event_buffer_idx = 0;

        count++;
        if(count == event_buffer_nums)
            return NULL;
    }

    event_buffer[event_buffer_idx].event_id = event_id;
    event_buffer[event_buffer_idx].event_flags = event_flags;
    event_buffer[event_buffer_idx].event_data = event_data;

    return &event_buffer[event_buffer_idx];
}

int32_t x_easyapp_event_delete(easyapp_event_t* event)
{
    if(event == NULL)
        return -1;

    event->event_id = 0;
    event->event_data = 0;
    event->event_flags = 0;

    return 0;
}

int32_t x_easyapp_event_send(easyapp_event_t *event)
{
    if(event == NULL)
        return -1;

    if((eventqueue.Tail + 1) % event_queue_nums == eventqueue.Head)
        return -2;

    eventqueue.eventqueue[eventqueue.Tail] = event;

    eventqueue.Tail++;
    if(eventqueue.Tail == event_queue_nums)
        eventqueue.Tail = 0;

    return 0;
}

