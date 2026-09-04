#ifndef __EASYAPP_PAGE_H__
#define __EASYAPP_PAGE_H__

#include <stdint.h>
#include <stddef.h>

typedef int8_t (*page_event_func)(void *);

typedef enum EASYAPP_REGISTERED_EVENTS
{
    ALL_PAGES_HEAD = -1,
    PAGE_EVENT_HEAD = 0,
    PAGE_EVENT_ONE,
    PAGE_EVENT_TWO,
    PAGE_EVENT_THREE,
    PAGE_EVENT_FOUR,
    PAGE_EVENT_FIVE,
    PAGE_EVENT_SIX,
    PAGE_EVENT_SEVEN,
    PAGE_EVENT_TAIL,
    ALL_PAGES_TAIL,
}EASYAPP_RIGISTERED_EVENTS_t;

typedef struct easyapp_page
{
    page_event_func func;
    uint32_t flag;
    EASYAPP_RIGISTERED_EVENTS_t page_event_lists[];
}easyapp_page_t;

#define easyapp_page_register(name, fun, ...)                             \
    __attribute__((used, section(".app_page_adr")))                       \
    easyapp_page_t easyapp_page_##name = {                                \
        .func = fun,                                                      \
        .flag = 0,                                                        \
        .page_event_lists = { __VA_ARGS__ }                               \
    }

extern EASYAPP_RIGISTERED_EVENTS_t ALL_PAGES_HEAD_;

extern EASYAPP_RIGISTERED_EVENTS_t ALL_PAGES_TAIL_;

int8_t easyapp_page_enable(easyapp_page_t * page);

int8_t easyapp_page_disable(easyapp_page_t * page);

#endif
