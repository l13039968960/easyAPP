#ifndef __EASYAPP_PAGE_H__
#define __EASYAPP_PAGE_H__

#include <stdint.h>
#include <stddef.h>

/**
 * @brief  页面事件处理函数原型
 * @param  event  事件对象(easyapp_event_t *), 页面只应读取
 * @return 1 = 事件已消费, 停止分发; 0 = 未消费, 继续发给后面的页面
 */
typedef int8_t (*page_event_func)(void *);

typedef enum page_flag{
    PAGE_FLAG_DISABLE,
    PAGE_FLAG_ENABLE,
}page_flag_t;

typedef enum EASYAPP_REGISTERED_EVENTS
{
    PAGE_EVENT_ONE,
    PAGE_EVENT_TWO,
    PAGE_EVENT_THREE,
    PAGE_EVENT_FOUR,
    PAGE_EVENT_FIVE,
    PAGE_EVENT_SIX,
    PAGE_EVENT_SEVEN,
    PAGE_EVENT_TAIL
}EASYAPP_RIGISTERED_EVENTS_t;

typedef struct easyapp_page
{
    page_event_func func;
    EASYAPP_RIGISTERED_EVENTS_t page_event_lists[];
}easyapp_page_t;

typedef struct easyapp_page_flag
{
    const easyapp_page_t* page_addr;
    page_flag_t flag;
}easyapp_page_flag_t;

/* 编译期约定 —— 这几条被 easyapp_core.c 的指针自增遍历和 stress_test.c 的
 * 按字扫描依赖, 改动任何一个都会静默破坏分发。放在这里让改动立刻编译失败。
 * (枚举 4 字节是 armclang 的默认行为; 若有人加了 -fshort-enums, 这里会报错。) */
_Static_assert(sizeof(EASYAPP_RIGISTERED_EVENTS_t) == 4, "事件枚举必须为 4 字节");
_Static_assert(sizeof(page_event_func) == 4,             "函数指针必须为 4 字节");
_Static_assert(sizeof(easyapp_page_flag_t) == 8,         "页标志必须为 8 字节");

/**
 * @brief  注册一个页面到 flash 页表
 * @param  name   页面名, 生成两个对象 easyapp_page_<name> / easyapp_page_flag_<name>
 * @param  fun    页面事件处理函数(page_event_func)
 * @param  state  初始使能状态(PAGE_FLAG_ENABLE / PAGE_FLAG_DISABLE)
 * @param  ...    该页订阅的事件列表, 必须以 PAGE_EVENT_TAIL 结尾
 */
#define easyapp_page_register(name, fun, state, ...)                      \
    __attribute__((used, section(".app_page_events")))                    \
    const easyapp_page_t easyapp_page_##name = {                          \
        .func = fun,                                                      \
        .page_event_lists = { __VA_ARGS__ }                               \
    };                                                                    \
    __attribute__((used, section(".app_page_flag")))                      \
    easyapp_page_flag_t easyapp_page_flag_##name = {                      \
        .page_addr = &easyapp_page_##name,                                \
        .flag = state                                                     \
    }

/**
 * @brief  把页面提到分发表最前, 优先接收事件
 * @param  page  页面标志对象
 * @retval 0     成功(已在最前时也返回 0, 不做任何改动)
 * @retval -1    page 为 NULL, 或不是数组里的真实槽位(锚点/野指针)
 * @note   只改变分发的遍历起点, 页面记录本身不搬动, 其余页面保持注册顺序。
 *         同时最多只有一个页面被提升 —— 它是"单槽"的: 连续提升 A 再 B,
 *         只有 B 在前, A 回到它原来的次序(不是 B,A 都在前)。
 *         重复提升同一页是幂等的。
 *         提升不改变使能状态: 被提升的页面照样要用 enable()/disable() 控制。
 * @note   在页面处理函数里调用它, 从**下一次** easyapp_core_run() 起生效 ——
 *         分发器在一次遍历开始时锁存起点, 中途改变不会影响正在进行的这一次。
 */
int8_t easyapp_page_to_front(easyapp_page_flag_t * page);

/**
 * @brief  使能页面, 开始接收事件
 * @param  page  页面标志对象
 * @retval 0     成功
 * @retval -1    page 为 NULL
 */
int8_t easyapp_page_enable(easyapp_page_flag_t * page);

/**
 * @brief  失能页面, 停止接收事件
 * @param  page  页面标志对象
 * @retval 0     成功
 * @retval -1    page 为 NULL
 */
int8_t easyapp_page_disable(easyapp_page_flag_t * page);

#endif
