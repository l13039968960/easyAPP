# EasyAPP

面向MCU的 **事件驱动框架**

核心思路：用 **放在 flash 里的静态页表**描述"哪些页面（页面处理函数）订阅了哪些事件"，中断/任务里只负责往**环形事件队列**塞事件，主循环调用一次分发器，事件就被送到订阅了它的页面处理函数手里。

---

## 特性

- **零动态分配**：事件队列为静态环形队列，页表为 flash 静态对象。
- **表与状态分离**：只读的页记录留在 flash，可写的使能状态单独放在 RAM，互不干扰（详见"核心设计"）。
- **页表自定界**：RAM 侧用"头锚点 + 尾锚点"标出边界，靠链接器把三段自定义段按序连续摆放，运行时遍历无需维护任何链表。
- **事件驱动分发**：一个事件可被多个页面订阅，按页表顺序逐个派发，直到某个页面返回"已消费"。
- **优先级抢占（轻量）**：`easyapp_page_to_front()` 让某个页面优先收到事件，只改一个遍历起点指针，不搬动任何记录，也不影响页面身份。
- **平台适配分层**：事件入队统一走 `easyapp_port` 包装，临界区实现可整体替换（裸机关中断 / RTOS 用 OS 临界区）。

---

## 目录结构

```
EasyAPP/
├── inc/                      # 公共头文件
│   ├── easyapp_page.h        # 页记录/页标志结构、注册宏、事件枚举、页面使能
│   ├── easyapp_event.h       # 事件结构、环形队列接口
│   └── easyapp_core.h        # 核心分发入口
│                              ※ 注：事件枚举目前仍定义在 easyapp_page.h，
│                                未来可考虑下沉到独立的公共头，供各层共用
├── src/                      # 实现
│   ├── easyapp_page.c        # 页标志锚点对象、使能/失能/提至最前
│   ├── easyapp_event.c       # 环形队列 send/get
│   └── easyapp_core.c        # 分发器（取事件 → 遍历页标志 → 派发）
├── port/                     # 平台适配层
│   ├── easyapp_port.h        # 带临界区保护的事件发送接口
│   └── easyapp_port.c        # 临界区实现（⚠️ 目前是空占位，见"已知限制"）
└── demo/MDK-ARM/             # KEIL工程（未入库）
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
                                                    页面处理函数(flash 页记录)
```

| 模块 | 职责 |
|---|---|
| `easyapp_event` | 环形事件队列，`x_easyapp_event_send()` 入队、`sp_easyapp_event_get()` 出队。不关心"页面"。 |
| `easyapp_page`   | flash 页记录 + RAM 页标志两张表：`easyapp_page_register()` 注册页、`enable/disable` 控制使能、`to_front` 提优先级、头尾锚点对象。 |
| `easyapp_core`   | 分发器：取一个事件，遍历页标志数组，找到订阅该事件的**启用**页面并调用其处理函数。 |
| `easyapp_port`   | 平台适配：事件发送时的临界区保护。 |

---

## 核心设计

### 1) 注册事件枚举

`inc/easyapp_page.h` 中的枚举即**整个系统已知的事件集合**：

```c
typedef enum EASYAPP_REGISTERED_EVENTS
{
    PAGE_EVENT_ONE,          /* 事件 ID 从 0 开始               */
    PAGE_EVENT_TWO,
    …
    PAGE_EVENT_SEVEN,
    PAGE_EVENT_TAIL,         /* 单个页面的订阅列表结束标记       */
} EASYAPP_RIGISTERED_EVENTS_t;
```

事件 `ID` 保存在事件结构里，也是页面订阅列表中元素的类型。**新增事件插在 `PAGE_EVENT_SEVEN` 与 `PAGE_EVENT_TAIL` 之间；`PAGE_EVENT_TAIL` 不可用作事件 ID。**

### 2) 表与状态分离：两张表，两种存储

这是当前架构和早期版本最大的不同。每个页面拆成**两个对象**：

| 对象 | 类型 | 存放处 | 段名 | 可写 |
|---|---|---|---|---|
| 页记录 `easyapp_page_<name>` | `const easyapp_page_t` | **flash** | `.app_page_events` | ❌ |
| 页标志 `easyapp_page_flag_<name>` | `easyapp_page_flag_t` | **RAM** | `.app_page_flag` | ✅ |

```c
/* flash：处理函数 + 变长的订阅列表（柔性数组） */
typedef struct easyapp_page {
    page_event_func            func;                 /* 4B */
    EASYAPP_RIGISTERED_EVENTS_t page_event_lists[];  /* 变长，以 PAGE_EVENT_TAIL 结尾 */
} easyapp_page_t;

/* RAM：指针 + 可写的使能状态 */
typedef struct easyapp_page_flag {
    const easyapp_page_t* page_addr;   /* 4B，指回 flash 里的页记录 */
    page_flag_t           flag;        /* 4B，PAGE_FLAG_DISABLE / ENABLE */
} easyapp_page_flag_t;
```

为什么要拆：页记录是常量，本可以在 flash 里带着使能标志一起放（早期版本就是这么做的）；但**使能状态需要运行时改写**，而 flash 不可随机写。拆开之后 flash 侧全部 `const`，RAM 侧只有 8 字节/页，且分发遍历只碰 RAM（快且无 flash 等待）。

### 3) 三段自定义段与分散加载（scatter）

分发器是**指针自增遍历**，所以 RAM 侧三段的相对顺序是硬约束：

```c
/* 被提升的页面先走一次, 然后要在循环里跳过它, 否则它会被派发两次 */
if (easyapp_page_flag_front != NULL &&
    easyapp_page_flag_front->flag == PAGE_FLAG_ENABLE)
{
    …派发 easyapp_page_flag_front…
}

page_ptr = &easyapp_page_flag_head;
while (page_ptr != &easyapp_page_flag_tail)
{
    if (page_ptr != easyapp_page_flag_front &&
        page_ptr->flag == PAGE_FLAG_ENABLE) { … }
    page_ptr++;
}
```

| 段 | 对象 | 含义 |
|---|---|---|
| `.app_page_flag_head` | `easyapp_page_flag_head` | 表起点锚点 |
| `.app_page_flag`      | `easyapp_page_flag_<name>` × N | 各页标志，每个 8B |
| `.app_page_flag_tail` | `easyapp_page_flag_tail` | 表终点锚点 |

RAM 布局（必须连续、按此顺序）：

```
 [ head:NULL,DIS ][ P_0:E ][ P_1:E ] … [ P_23:E ][ tail:NULL,DIS ]
   8B 头锚点        8B       8B            8B       8B 尾锚点
```

flash 侧记录变长，靠 `PAGE_EVENT_TAIL` 定界，运行时**不线性扫描**（通过 `page_addr` 跳转），所以只要求连续、顺序摆放：

```
 [ func|e0|e1|…|TAIL ][ func|e0|e1|TAIL ] …
   12B / 20B / 24B …，每条以 TAIL 收尾
```

**必须靠 scatter 文件钉死**，默认链接布局不保证这两个顺序。参考 `demo/MDK-ARM/MDK-ARM/EASYAPP.sct`：

```
LR_IROM1 0x08000000 0x0007F800  {   ; 主程序
  ER_IROM1 0x08000000 0x0007F800  { *.o (RESET, +First)
                                    *(InRoot$$Sections)
                                    .ANY (+RO)  .ANY (+XO) }
  RW_IRAM1 0x20000000 0x0001FC00  { .ANY (+RW +ZI) }
  EASYAPP_RAM 0x2001FC00 0x00000400  {          ; 页标志数组
    *(.app_page_flag_head, +First)
    *(.app_page_flag)
    *(.app_page_flag_tail, +Last)
  }
}

LR_PAGES 0x0807F800 0x00000800  {   ; 页表（独立加载区, load == exec）
  ER_PAGES 0x0807F800 0x00000800  { *(.app_page_events) }
}
```

两个易踩的点：

- **`+First` / `+Last` 是排序选择器，不是绝对地址锚定。** 它们只保证"在区域内排最前/最后"，段之间不留空洞由选择器本身的相邻性保证。
- **页表单独开一个加载区（`LR_PAGES`）而不是塞进 `LR_IROM1` 当执行区。** 塞进去会让 armlink 给出 `Load base ≠ Exec base`，于是自动生成一条拷贝表项，**开机时多一次 flash→flash 拷贝**。独立加载区让 `load == exec`，页表就地存放，烧录后在固定地址直接可见。

### 4) 环形事件队列

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

两个语义要记住：

- **按值入队，但 `event_data` 只是个指针，指向的数据不会被复制。** 发送方必须保证该内存在事件被消费前一直有效（全局/静态缓冲，或在消费者里拷贝走）。
- **`sp_easyapp_event_get()` 返回的指针指向队列内部**，下一次 `get()` 之后内容就可能被覆盖，不要长期持有。

### 5) 分发流程（`easyapp_core_run`）

1. `sp_easyapp_event_get()` 取出一个事件；空队列直接返回。
2. 若存在被提升的页面（`easyapp_page_flag_front`）且它已使能，先单独派发它一次 —— 被消费则直接返回。
3. 从 `&easyapp_page_flag_head` 开始逐条扫描页标志数组，直到 `&easyapp_page_flag_tail`。
4. 每条先看 `flag`，跳过未使能的页面；**并且跳过第 2 步已经派发过的那个页面**。
5. 沿 `page_addr` 找到 flash 页记录，在订阅列表里逐项比对事件 `ID`。
6. 命中 → 调用 `func(event)`：
   - 返回 `1`：事件被消费，`easyapp_core_run()` 立即返回，后续页面收不到。
   - 返回 `0`：不消费，继续比对**同一个页面的剩余订阅项**和后续页面（一个事件可被多个页面先后处理）。

注意第 6 步的细节：**订阅表是扫完的，不是命中即停**。所以一个页面订阅 N 个事件，每次派发都要做 N 次比较，成本是 `O(页数 × 表长)` / 事件。这也是当前性能的主要开销来源。

### 6) 编译期约定（`_Static_assert`）

`easyapp_page.h` 里钉了三条断言，改动任何一个都会**立刻编译失败**而不是静默跑错：

```c
_Static_assert(sizeof(EASYAPP_RIGISTERED_EVENTS_t) == 4, "事件枚举必须为 4 字节");
_Static_assert(sizeof(page_event_func) == 4,             "函数指针必须为 4 字节");
_Static_assert(sizeof(easyapp_page_flag_t) == 8,         "页标志必须为 8 字节");
```

第一条尤其重要：它靠枚举是 **4 字节**才能让 `page_ptr++` 的步长、以及按 `uint32_t` 视角扫描订阅表的代码成立。**AC6/Keil 下必须关掉 "Short enums/wchar"（`<vShortEn>0</vShortEn>`）**，否则枚举变 1 字节，断言会直接报错。

---

## 使用示例

```c
#include "easyapp_page.h"
#include "easyapp_event.h"
#include "easyapp_core.h"
#include "easyapp_port.h"

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

/* 注册页面：宏会一次生成两个对象
 *   easyapp_page_menu_page      (flash 页记录, .app_page_events)
 *   easyapp_page_flag_menu_page (RAM 页标志, .app_page_flag) */
easyapp_page_register(menu_page, menu_page_on_event,
                      PAGE_FLAG_ENABLE,      /* 初始使能状态 */
                      PAGE_EVENT_ONE,
                      PAGE_EVENT_TWO,
                      PAGE_EVENT_TAIL);      /* 列表必须以此结尾 */

int main(void)
{
    /* 页面已在注册时使能；需要动态控制时用下面这几个 */
    easyapp_page_disable(&easyapp_page_flag_menu_page);
    easyapp_page_enable (&easyapp_page_flag_menu_page);
    easyapp_page_to_front(&easyapp_page_flag_menu_page);  /* 提到分发序列最前 */

    for (;;) {
        easyapp_core_run();                          /* 主循环轮询分发 */
    }
}

/* 中断/任务里发送事件（带临界区保护，见 port 层） */
void some_irq(void)
{
    x_port_easyapp_event_send(PAGE_EVENT_ONE, 0, NULL);
}
```

> `enable/disable/to_front` 收的都是**页标志对象**（RAM，可写），不是 flash 里的页记录 —— 页记录是 `const`，传错会编译报错。

`easyapp_page_to_front()` **只改分发的遍历起点，不搬动任何东西**：页面记录留在原槽位，使能状态也留在原槽位，变的只是 `easyapp_page_flag_front` 这一个指针。

这一点有个必须守住的理由：**页面身份 = 对象地址**。`easyapp_page_flag_<name>` 是稳定的具名对象，调用方拿它当"这个页面"的句柄 —— 如果 `to_front` 把记录挪来挪去，这个句柄就退化成"这个槽位上当前坐着谁"，`enable`/`disable`/`to_front` 会全部作用到别的页面上。

早期版本借头锚点槽位当优先级槽（把头锚点改成"指向该页 + 使能"，同时关掉该页自己的标志位），那会让**使能状态同时存在两份**：一份在头槽、一份在原槽。后果是 `to_front` 重复调用同一页会把刚关掉的原槽又打开（派发两次），而 `disable()` 够不着头槽那份（提升过的页关不掉）。现在状态只有 `easyapp_page_flag_front` 这一个指针，两个问题都不存在，头锚点也永远不会被写入（其 `page_addr` 恒为 `NULL`）。

**它是"单槽"的**：同时只记得一个被提升的页面，所以连续提升 A 再 B 之后只有 B 在前，A 回到原来的次序。

代价是遍历循环**每个条目多一次 `!= front` 比较**。反汇编上是 2 条指令（`CMP` + `BEQ`，循环体 6 → 8 条）；分发器在遍历开始时把起点锁存进寄存器，所以没有"每个条目重载一次全局量"的额外开销。**把订阅扫描抽成 `easyapp_dispatch_to()` 不花钱** —— armclang 会把它完全内联回 `easyapp_core_run`（实测内联后与内联写法耗时逐位相同）。
