#include "../inc/easyapp_event.h"

static easyapp_eventqueue_t eventqueue;

easyapp_event_t* sp_easyapp_event_get(void)
{
    if(eventqueue.Head == eventqueue.Tail)
        return NULL; //NULL

     easyapp_event_t* event = &eventqueue.eventqueue[eventqueue.Head];

     eventqueue.Head++;
     if(eventqueue.Head == event_queue_nums)
        eventqueue.Head = 0;

    return event;
}

int32_t x_easyapp_event_send(EASYAPP_RIGISTERED_EVENTS_t event_id, uint32_t event_flags, void *event_data)
{
    if((eventqueue.Tail + 1) % event_queue_nums == eventqueue.Head)
        return -1; //FULL

    eventqueue.eventqueue[eventqueue.Tail].event_id = event_id;
    eventqueue.eventqueue[eventqueue.Tail].event_flags = event_flags;
    eventqueue.eventqueue[eventqueue.Tail].event_data = event_data;

    eventqueue.Tail++;
    if(eventqueue.Tail == event_queue_nums)
        eventqueue.Tail = 0;

    return 0;
}

