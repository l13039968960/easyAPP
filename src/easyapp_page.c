#include "./inc/easyapp_page.h"

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

