#ifndef __EASYAPP_EVENT_H__
#define __EASYAPP_EVENT_H__

#include <stdint.h>
#include <stddef.h>
#include "easyapp_page.h"

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

    easyapp_event_t eventqueue[event_queue_nums];
}easyapp_eventqueue_t;

/**
 * @brief  事件入队
 * @param  event_id     事件 ID
 * @param  event_flags  自定义标志位, 原样带给处理函数
 * @param  event_data   数据指针, 可为 NULL
 * @retval 0     成功
 * @retval -1    队列满
 * @note   本函数不加临界区保护, 中断里请用 x_port_easyapp_event_send()
 */
int32_t x_easyapp_event_send(EASYAPP_RIGISTERED_EVENTS_t event_id, uint32_t event_flags, void *event_data);

/**
 * @brief  取出一个事件
 * @return 队列内的事件指针; 队列空返回 NULL
 * @note   由 easyapp_core_run() 调用。返回的指针指向队列内部,
 *         下次 get 之后内容才会被覆盖, 不要长期持有
 */
easyapp_event_t* sp_easyapp_event_get(void);


#endif
