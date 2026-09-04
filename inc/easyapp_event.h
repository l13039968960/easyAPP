#ifndef __EASYAPP_EVENT_H__
#define __EASYAPP_EVENT_H__

#include <stdint.h>
#include <stddef.h>
#include "easyapp_page.h"

#define event_buffer_nums 256
#define event_queue_nums 128

typedef struct easyapp_event
{
    EASYAPP_RIGISTERED_EVENTS_t event_id;
    uint32_t event_flags;

    void* event_data;
}easyapp_event_t;

typedef struct easyapp_eventqueue
{
    uint32_t Head;
    uint32_t Tail;

    easyapp_event_t* eventqueue[event_queue_nums];
}easyapp_eventqueue_t;

easyapp_event_t* sp_easyapp_event_create(EASYAPP_RIGISTERED_EVENTS_t event_id, uint32_t event_flags, void *event_data);

int32_t x_easyapp_event_delete(easyapp_event_t* event);

int32_t x_easyapp_event_send(easyapp_event_t *event);

easyapp_event_t* sp_easyapp_event_get(void);


#endif
