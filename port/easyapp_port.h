#ifndef EASYAPP_PORT_H__
#define EASYAPP_PORT_H__

#include "../inc/easyapp_page.h"
#include "../inc/easyapp_core.h"

/*
 * 运行环境选择:
 *   OSSUPPORT = 1  -> 运行在 RTOS 下, 临界区使用 OS 自带接口(见 easyapp_port.c 顶部)
 *   OSSUPPORT = 0  -> 裸机, 临界区使用"关中断"(Cortex-M PRIMASK, 保存/恢复)
 */
#define OSSUPPORT   0

int32_t x_port_easyapp_event_send(EASYAPP_RIGISTERED_EVENTS_t event_id, uint32_t event_flags, void *event_data);

#endif
