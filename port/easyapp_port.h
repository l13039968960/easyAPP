#ifndef EASYAPP_PORT_H__
#define EASYAPP_PORT_H__

#include "../inc/easyapp_page.h"
#include "../inc/easyapp_core.h"


/**
 * @brief  发送事件(带临界区保护)
 * @param  event_id     事件 ID
 * @param  event_flags  自定义标志位, 原样带给处理函数
 * @param  event_data   数据指针, 可为 NULL
 * @retval 0     成功
 * @retval -1    队列满
 * @note   中断 / 任务 / 主循环里都可以调用, 是事件入队的唯一推荐入口
 */
int32_t x_port_easyapp_event_send(EASYAPP_RIGISTERED_EVENTS_t event_id, uint32_t event_flags, void *event_data);

#endif
