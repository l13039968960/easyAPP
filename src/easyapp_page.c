#include "./inc/easyapp_page.h"

__attribute__((used, section(".app_page_flag_head")))
easyapp_page_flag_t easyapp_page_flag_head = {
        .page_addr = NULL,
        .flag = PAGE_FLAG_DISABLE  
};

__attribute__((used, section(".app_page_flag_tail")))
easyapp_page_flag_t easyapp_page_flag_tail = {
        .page_addr = NULL,
        .flag = PAGE_FLAG_DISABLE  
};

/* 被提升到最前的页面, NULL = 不提升。只被 easyapp_core.c 读。
 *
 * 提升**不搬动任何东西**: 页面记录留在原槽位, 使能状态也留在原槽位,
 * 变的只是分发的遍历起点。这样做有两个必须的理由:
 *   1. 页面身份 = 对象地址。easyapp_page_flag_<name> 是个稳定的具名对象,
 *      调用方拿它当"这个页面"的句柄; 一旦记录会搬家, 这个句柄就退化成
 *      "这个槽位上当前坐着谁", enable/disable/to_front 全部失去意义。
 *   2. 使能状态只能有一份。之前的写法把头锚点槽位借出去当优先级槽,
 *      使能状态就同时存在头槽和原槽两份, 于是:
 *        - 对同一页重复提升时, 恢复上一条的代码会把刚关掉的原槽又打开;
 *        - 头槽那份没有任何出口, disable() 够不着, 提升过的页关不掉。
 * 现在状态只有 easyapp_page_flag_front 这一个指针, 上面两个问题都不存在,
 * 头锚点也永远不会被写入(其 page_addr 恒为 NULL)。 */
easyapp_page_flag_t* easyapp_page_flag_front = NULL;

int8_t easyapp_page_to_front(easyapp_page_flag_t* page)
{
    easyapp_page_flag_t* first = &easyapp_page_flag_head + 1;

    /* 只接受落在页标志数组内的真实槽位; 锚点、野指针一律拒绝。
     * (一个页面都没注册时 first == &tail, 这里会把所有输入都拦下。) */
    if(page == NULL || page < first || page >= &easyapp_page_flag_tail)
    {
        return -1;
    }

    /* 重复提升同一页只是把同一个指针写回去, 天然幂等 */
    easyapp_page_flag_front = page;

    return 0;
}

int8_t easyapp_page_enable(easyapp_page_flag_t * page)
{
    if(page == NULL)
        return -1;
    page->flag = PAGE_FLAG_ENABLE;

    return 0;
}

int8_t easyapp_page_disable(easyapp_page_flag_t * page)
{
    if(page == NULL)
        return -1;
    page->flag = PAGE_FLAG_DISABLE;

    return 0;
}

