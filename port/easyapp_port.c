#include <stdint.h>
#include "easyapp_port.h"
#include "../inc/easyapp_event.h"

/*
 * =====================================================================
 *  临界区实现选择 (由 easyapp_port.h 里的 OSSUPPORT 决定)
 * =====================================================================
 */
#if OSSUPPORT
    /* 
     * FreeRTOS 示例 (需要 #include "FreeRTOS.h" 等, 且不能在 ISR 里调用!):
     *   #define EASYAPP_ENTER_CRITICAL()    taskENTER_CRITICAL()
     *   #define EASYAPP_EXIT_CRITICAL()     taskEXIT_CRITICAL()
     *
     * CMSIS-RTOS2 / RTX5 示例:
     *   #define EASYAPP_ENTER_CRITICAL()    (void)osKernelLock()
     *   #define EASYAPP_EXIT_CRITICAL()     (void)osKernelUnlock()
     */
    #ifndef EASYAPP_ENTER_CRITICAL
    #error "OSSUPPORT=1: 请先在 easyapp_port.c 顶部把 EASYAPP_ENTER/EXIT_CRITICAL 映射为你的 RTOS 接口"
    #endif
#else
    /* ---------------- 裸机: 关中断即可 (Cortex-M) ----------------
     * __get_PRIMASK / __disable_irq / __set_PRIMASK 由 CMSIS 内核头(core_cm*.h)提供。
     * Keil 工程通常已通过器件头(如 stm32f4xx.h)间接包含; 若本文件报未定义,
     * 就在本文件顶部 #include 你器件的 CMSIS 头。
     */
#endif/*OSSUPPORT*/

int32_t x_port_easyapp_event_send(EASYAPP_RIGISTERED_EVENTS_t event_id, uint32_t event_flags, void *event_data)
{
    int32_t ret;

#if OSSUPPORT
    EASYAPP_ENTER_CRITICAL();
#else
    uint32_t primask = __get_PRIMASK();   /* 保存旧的中断屏蔽状态 */
    __disable_irq();                       /* 关中断 */
#endif

    ret = x_easyapp_event_send(event_id, event_flags, event_data);

#if OSSUPPORT
    EASYAPP_EXIT_CRITICAL();
#else
    __set_PRIMASK(primask);                /* 恢复旧状态(不是__enable_irq), 嵌套调用也安全 */
#endif

    return ret;
}
