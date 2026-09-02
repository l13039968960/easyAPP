#ifndef __EASYAPP_PAGE_H__
#define __EASYAPP_PAGE_H__

#include <stdint.h>

typedef void *(*page_event_func)(void *);

typedef enum EASYAPP_REGISTERED_EVENTS
{
    PAGE_EVENT_HEAD = 0x00000000,
    PAGE_EVENT_ONE,
    PAGE_EVENT_TWO,
    PAGE_EVENT_THREE,
    PAGE_EVENT_FOUR,
    PAGE_EVENT_FIVE,
    PAGE_EVENT_SIX,
    PAGE_EVENT_SEVEN,
    PAGE_EVENT_TAIL = 0xFFFFFFFF,
}EASYAPP_RIGISTERED_EVENTS_t;

typedef struct easyapp_page
{
    page_event_func func;
    uint32_t flag;
    EASYAPP_RIGISTERED_EVENTS_t page_event_lists[];
}easyapp_page_t;

#define easyapp_page_register(name, fun, ...)                             \
    __attribute__((used, section(".page_adr")))                           \
    easyapp_page_t easyapp_page_##name = {                                \
        .func = fun,                                                      \
        .flag = 0,                                                        \
        .page_event_lists = { __VA_ARGS__ }                               \
    }

int8_t easyapp_page_enable(easyapp_page_t * page);

int8_t easyapp_page_disable(easyapp_page_t * page);

#endif
