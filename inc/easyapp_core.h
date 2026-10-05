#ifndef __EASYAPP_CORE_H__
#define __EASYAPP_CORE_H__

/**
 * @brief  事件分发: 取一个事件, 遍历页表, 调用订阅它的已使能页面
 * @note   在 main 循环里反复调用。队列空时立即返回, 不会阻塞
 */
void easyapp_core_run(void);

#endif
