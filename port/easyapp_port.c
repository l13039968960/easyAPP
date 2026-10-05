#include <stdint.h>
#include "easyapp_port.h"
#include "../inc/easyapp_event.h"


int32_t x_port_easyapp_event_send(EASYAPP_RIGISTERED_EVENTS_t event_id, uint32_t event_flags, void *event_data)
{
    int32_t ret;

    /*互斥实现*/

    ret = x_easyapp_event_send(event_id, event_flags, event_data);

    /*互斥实现*/

    return ret;
}
