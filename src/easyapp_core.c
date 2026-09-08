#include "./inc/easyapp_core.h"
#include "./inc/easyapp_page.h"
#include "./inc/easyapp_event.h"

#include <stddef.h>

extern EASYAPP_RIGISTERED_EVENTS_t ALL_PAGES_HEAD_;
extern EASYAPP_RIGISTERED_EVENTS_t ALL_PAGES_TAIL_;

/*消费一个事件*/
void easyapp_core_run(void)
{
    page_event_func ptr_fun;
    uint32_t page_flag;
    uint32_t *page_ptr;
    easyapp_event_t* active_event;

    active_event = sp_easyapp_event_get();

    if(active_event == NULL)
        return;
    
    // NOT NULL
    page_ptr = (uint32_t *)&ALL_PAGES_HEAD_;
    page_ptr ++;

    while(*(EASYAPP_RIGISTERED_EVENTS_t*)page_ptr != ALL_PAGES_TAIL_)
    {
        ptr_fun = *(page_event_func *)page_ptr;
        
        page_ptr ++;
        page_flag = *page_ptr;
        
        page_ptr ++;
        while(*(EASYAPP_RIGISTERED_EVENTS_t*)page_ptr != PAGE_EVENT_TAIL)
        {
            if(page_flag != 0)
            {
                if(*(EASYAPP_RIGISTERED_EVENTS_t*)page_ptr == active_event->event_id)
                {
                    int8_t return_flag = ptr_fun((void*)active_event);
                    if(return_flag == 1) //consume
                        return;
                }
            }
            page_ptr ++;
        }
        page_ptr ++;
    }
    
}