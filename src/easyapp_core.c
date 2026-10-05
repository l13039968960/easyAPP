#include "./inc/easyapp_core.h"
#include "./inc/easyapp_page.h"
#include "./inc/easyapp_event.h"

#include <stddef.h>

extern easyapp_page_flag_t easyapp_page_flag_head;
extern easyapp_page_flag_t easyapp_page_flag_tail;

/* 被提升到最前的页面, NULL = 不提升。由 easyapp_page_to_front() 设置 */
extern easyapp_page_flag_t* easyapp_page_flag_front;

/* 把事件交给一个页面: 扫它的订阅表, 命中就调用处理函数。
 * 返回 1 = 事件已被消费, 分发应当立刻停止。 */
static int8_t easyapp_dispatch_to(easyapp_page_flag_t* page, easyapp_event_t* event)
{
    const EASYAPP_RIGISTERED_EVENTS_t* event_ptr = page->page_addr->page_event_lists;

    while(*event_ptr != PAGE_EVENT_TAIL)
    {
        if(*event_ptr == event->event_id)
        {
            if(page->page_addr->func((void*)event) == 1) //consume
                return 1;
        }
        event_ptr++;
    }

    return 0;
}

void easyapp_core_run(void)
{
    easyapp_page_flag_t *page_ptr;
    easyapp_page_flag_t *front;
    easyapp_event_t* active_event;

    active_event = sp_easyapp_event_get();
    if(active_event == NULL)
        return;

    /* 在本次遍历开始时锁存, 之后不再读全局量。这样处理函数里调 to_front()
     * 不会在同一次遍历中途改变顺序 —— 那种行为很难说清, 也没人需要;
     * 现在是明确的"从下一次派发生效"。顺带把这次读提到循环外,
     * 省掉每个条目重新加载一次全局量。 */
    front = easyapp_page_flag_front;

    /* 被提升的页面先走一次。它自身的位置没有动, 所以下面遍历到它时必须跳过,
     * 否则会被派发两次。使能状态和别的页面一样只看它自己的 flag ——
     * 于是 easyapp_page_disable() 对提升过的页面同样有效。 */
    if(front != NULL && front->flag == PAGE_FLAG_ENABLE)
    {
        if(easyapp_dispatch_to(front, active_event) == 1)
            return;
    }

    page_ptr = &easyapp_page_flag_head;

    while(page_ptr != &easyapp_page_flag_tail)
    {
        if(page_ptr != front && page_ptr->flag == PAGE_FLAG_ENABLE)
        {
            if(easyapp_dispatch_to(page_ptr, active_event) == 1)
                return;
        }
        page_ptr++;
    }

}