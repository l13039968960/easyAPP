#include "./inc/easyapp_page.h"

__attribute__((used, section(".app_page_head")))
EASYAPP_RIGISTERED_EVENTS_t ALL_PAGES_HEAD_ = ALL_PAGES_HEAD;

__attribute__((used, section(".app_page_tail")))
EASYAPP_RIGISTERED_EVENTS_t ALL_PAGES_TAIL_ = ALL_PAGES_TAIL;

int8_t easyapp_page_enable(easyapp_page_t * page)
{
    if(page == NULL)
        return -1;
    page->flag = 1;

    return 0;
}

int8_t easyapp_page_disable(easyapp_page_t * page)
{
    if(page == NULL)
        return -1;
    page->flag = 0;

    return 0;
}

