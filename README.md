# EasyAPP

一个面向单片机的 **页面（Page）— 事件（Event）驱动框架**

核心思路：用 **放在 flash 里的静态页表**描述"哪些页面（页面处理函数）订阅了哪些事件"，中断/任务里只负责往**环形事件队列**塞事件，主循环调用一次分发器，事件就被送到订阅了它的页面处理函数手里。

> 状态：**框架骨架，开发中**。当前仓库不含具体芯片工程与 app 层示例，接入 Keil 需要完成文末"接入与配置"一节。

---

## 特性

- **零动态分配**：事件队列为静态环形队列，页表为 flash 静态对象。
- **页表自定界**：页表用"头锚点 + 尾锚点"标出边界，靠链接器把三个自定义段按序连续摆放，运行时遍历无需维护任何链表。
- **事件驱动分发**：一个事件可被多个页面订阅，按页表顺序逐个派发，直到某个页面返回"已消费"。
- **裸机 / RTOS 均可**：发送接口通过 `easyapp_port` 切换临界区实现（裸机关中断，RTOS 用 OS 临界区）。

---

## 目录结构

```
EasyAPP/
├── inc/                      # 公共头文件
│   ├── easyapp_page.h        # 页表结构、注册宏、事件枚举、页面使能
│   ├── easyapp_event.h       # 事件结构、环形队列接口
│   └── easyapp_core.h        # 核心分发入口
│                              ※ 注：事件枚举目前仍定义在 easyapp_page.h，
│                                未来可考虑下沉到独立的公共头，供各层共用
├── src/                      # 实现
│   ├── easyapp_page.c        # 页表锚点对象、页面使能/失能
│   ├── easyapp_event.c       # 环形队列 send/get
│   └── easyapp_core.c        # 分发器（消费事件 → 遍历页表 → 派发）
└── port/                     # 平台适配层
    ├── easyapp_port.h        # OSSUPPORT 环境开关
    └── easyapp_port.c        # 带临界区保护的事件发送包装
```

---

## 分层与数据流

```
  中断 / 任务(生产者)                主循环 / 任务(消费者)
        │   x_port_easyapp_event_send()       │  easyapp_core_run()
        ▼                                      ▼
   ┌──────────────┐  入队   ┌──────────────┐  出队   ┌──────────────────┐
   │  easyapp_port │ ─────▶ │ easyapp_event │ ─────▶ │  easyapp_core     │
   │ (临界区包装)   │        │ (环形队列)      │        │ 取1个事件→查页表    │
   └──────────────┘        └──────────────┘        └─────────┬────────┘
                                                             │ 命中订阅
                                                             ▼
                                                    页面处理函数(页表 .app_page_adr)
```

| 模块 | 职责 |
|---|---|
| `easyapp_event` | 环形事件队列，`x_easyapp_event_send()` 入队、`sp_easyapp_event_get()` 出队。不关心"页面"。 |
| `easyapp_page`   | flash 页表：`easyapp_page_register()` 注册页、`enable/disable` 控制使能、头尾锚点对象。 |
| `easyapp_core`   | 分发器：取一个事件，遍历页表，找到订阅该事件的**启用**页面并调用其处理函数。 |
| `easyapp_port`   | 平台适配：事件发送时的临界区保护（裸机 / OS 二选一）。 |

---

## 核心设计

### 1) 注册事件枚举

`inc/easyapp_page.h` 中的枚举即**整个系统已知的事件集合**：

```c
typedef enum EASYAPP_REGISTERED_EVENTS
{
    ALL_PAGES_HEAD = -1,     /* 页表头锚点取值（区域起始标记）   */
    PAGE_EVENT_HEAD = 0,     /* 事件 ID 从 0 开始               */
    PAGE_EVENT_ONE,          /* …可在此按需继续扩展事件…          */
    …
    PAGE_EVENT_TAIL,         /* 单个页面的订阅列表结束标记       */
    ALL_PAGES_TAIL,          /* 整张页表结束标记                 */
} EASYAPP_RIGISTERED_EVENTS_t;
```

事件 `ID` 保存在事件结构里，也是页面订阅列表中元素的类型。**新增事件在 `…SEVEN` 与 `PAGE_EVENT_TAIL` 之间插入，末尾两个标记值不可用作事件 ID。**

### 2) flash 页表与三段自定义段

每个"页面"是一个 `easyapp_page_t` 对象（普通函数指针 + 使能标志 + 柔性数组订阅列表），经注册宏放入自定义段：

```c
#define easyapp_page_register(name, fun, ...)                  \
    __attribute__((used, section(".app_page_adr")))            \
    easyapp_page_t easyapp_page_##name = {                     \
        .func = fun,                                           \
        .flag = 0,                                             \
        .page_event_lists = { __VA_ARGS__ }                    \
    }
```

`easyapp_page.c` 再放两个**锚点对象**作为整张表的边界：

| 段 | 对象 | 值 | 含义 |
|---|---|---|---|
| `.app_page_head` | `ALL_PAGES_HEAD_` | `ALL_PAGES_HEAD` | 表起点 |
| `.app_page_adr` | `easyapp_page_<name>` × N | — | 各页面记录 |
| `.app_page_tail` | `ALL_PAGES_TAIL_` | `ALL_PAGES_TAIL` | 表终点 |

约定内存布局（必须连续、按此顺序摆放）：

```
[ -1 | func|flag|订阅列表…|PAGE_EVENT_TAIL | func|flag|订阅列表…|PAGE_EVENT_TAIL | 9 ]
 ^     └───────────── 页面 1 ─────────────┘ └───────────── 页面 2 ─────────────┘  ^
 头锚点(.app_page_head)                                                  尾锚点(.app_page_tail)
```

每页一条记录，ARM 32 位下成员天然 4 字节对齐：`func`(4B) + `flag`(4B) + 事件列表(4B/个，末尾为 `PAGE_EVENT_TAIL`)。

### 3) 环形事件队列

```c
typedef struct easyapp_eventqueue
{
    uint32_t Head;
    uint32_t Tail;
    easyapp_event_t eventqueue[event_queue_nums];   /* 结构体数组, 按值存储 */
} easyapp_eventqueue_t;
```

- `event_queue_nums = 128`，最大容纳 **127** 个事件（保留 1 槽区分空/满）。
- 空：`Head == Tail`；满：`(Tail + 1) % N == Head`。
- `send()` 先写 `queue[Tail]` 再推进 `Tail`；`get()` 读 `queue[Head]` 再推进 `Head`，返回指向队内元素的指针（空队返回 `NULL`）。

### 4) 分发流程（`easyapp_core_run`）

1. `sp_easyapp_event_get()` 取出一个事件；空队列直接返回。
2. 从 `&ALL_PAGES_HEAD_ + 1` 开始逐条扫描页表。
3. 每条先读 `func`、`flag`，再在订阅列表里找与事件 `ID` 相同的项。
4. 命中且页面**已使能**（`flag != 0`）→ 调用 `func(event)`：
   - 返回 `1`：事件被消费，停止分发并返回。
   - 返回 `0`：不消费，继续匹配后续页面（一个事件可被多个页面先后处理）。

---

## 使用示例

```c
#include "easyapp_page.h"
#include "easyapp_event.h"
#include "easyapp_core.h"

/* 页面处理函数：返回 1 = 消费事件；0 = 继续向后传 */
int8_t menu_page_on_event(void *ev)
{
    easyapp_event_t *e = (easyapp_event_t *)ev;

    if (e->event_id == PAGE_EVENT_ONE) {
        /* …做页面该做的事… */
        return 1;              /* 已消费 */
    }
    return 0;
}

/* 注册页面到 flash 页表，订阅 HEAD、ONE 两个事件 */
easyapp_page_register(menu_page, menu_page_on_event,
                      PAGE_EVENT_HEAD,
                      PAGE_EVENT_ONE,
                      PAGE_EVENT_TAIL);     /* 列表必须以此结尾 */

int main(void)
{
    easyapp_page_enable(&easyapp_page_menu_page);   /* 启用页面 */

    for (;;) {
        easyapp_core_run();                          /* 主循环轮询分发 */
    }
}

/* 中断/任务里发送事件（带临界区保护，见 port 层） */
void some_irq(void)
{
    x_port_easyapp_event_send(PAGE_EVENT_ONE, 0, NULL);
}




