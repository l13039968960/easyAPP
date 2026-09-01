#ifndef __EASYAPP_EVENT_H__
#define __EASYAPP_EVENT_H__

#include <stdint>

typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned char uint8_t;

#define event_buffer_nums 256
#define event_queue_nums 128

typedef struct easyapp_event
{
    uint32_t event_id;
    uint8_t event_flags;

    void* event_data;
}easyapp_event_t;

typedef struct easyapp_eventqueue
{
    uint32_t Head;
    uint32_t Tail;

    easyapp_event_t* eventqueue[event_queue_nums];
}easyapp_eventqueue_t;

easyapp_event_t* sp_easyapp_event_create(uint32_t event_id, uint32_t event_flags, void *event_data);

int32_t x_easyapp_event_delete(easyapp_event_t* event);

int32_t x_easyapp_event_send(easyapp_event_t *event);


#endif
