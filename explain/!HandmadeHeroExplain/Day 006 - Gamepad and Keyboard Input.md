---
集数: "006"
英文标题: Gamepad and Keyboard Input
别名:
  - Day 6
  - 手柄与键盘
  - XInput
标签:
  - handmade-hero
  - day/006
  - 主题/输入
  - 主题/动态链接
  - 主题/平台层
相关代码: HandMade/code/win32_handmade.cpp
字幕: "explain/!subtitle/[Mira] Handmade Hero Day 006 - Gamepad and Keyboard Input.txt"
状态: 已完成
---

# Day 006 —— Gamepad and Keyboard Input(手柄与键盘输入)

> **本篇导航**:一、这一集干了什么(总览) · 二、XInput 是什么,以及**为什么非用它不可** · 三、轮询 vs 中断,以及那个 **packet number** · 四、`XINPUT_GAMEPAD` 逐字段(以及**老派游戏需要哪些**) · 五、★ 链接问题:为什么不直接链接 `xinput.lib` · 六、★ 自己动态加载:三件套 + 那个"宏技巧" · 七、`Win32LoadXInput()`:真正把它加载进来 · 八、★ 键盘输入:`WPARAM` / `LPARAM` 解密 · 九、⚠️ 结尾的自我更正(这条**直接影响我们的代码**) · 十、Q&A 要点 · 十一、和我们代码的关系 / 下一步(重要) · 附录 A–D

> **素材**:`D:\HandMade\explain\!subtitle\[Mira] Handmade Hero Day 006 - Gamepad and Keyboard Input.txt`
> (775 条字幕,3,099 行,约 **1 小时 32 分**;本文件按内容重排,不是逐句翻译)
> **本文件**:`D:\HandMade\explain\!HandmadeHeroExplain\Day 006 - Gamepad and Keyboard Input.md`
> **一句话总结**:**第一次真正拿到"人"的输入** —— 用 XInput 读手柄,而且为了"没有手柄也能玩",他
> **自己 `LoadLibrary` + `GetProcAddress` 动态加载** `xinput1_3.dll`(不链接 `xinput.lib`);
> 顺带把 `WM_KEYDOWN`/`WM_KEYUP` 里的 `WPARAM`/`LPARAM` 解密成可靠的布尔值。
> **结尾还有一条对上周做法的自我更正** —— 见第九节 ⚠️(会影响我们的代码)。

> **本周计划**:周一~周四做**输入**和**声音**;他说如果声音三天搞定,周五就做**文件 IO**,
> 那"下下周就能开始真正的游戏了"。他还感慨:**花两周做 Windows 层,比很多人调 CSS 不生效的时间还短**。

---

## 一、这一集干了什么(总览)

| 时间段 | 内容 |
|---|---|
| 0:00 ~ 2:00 | 开场:本周做**输入 + 声音**;Day 5 源码解压出来接着写 |
| 2:00 ~ 7:00 | **XInput 是什么**:微软先做了 DirectInput,几年后又做了 XInput,两者**不真正兼容** |
| 7:00 ~ 11:00 | **轮询 vs 中断**;XInput 是**轮询式** API → 必须每帧主动问一次 |
| 11:00 ~ 16:30 | 逐字段看 `XINPUT_GAMEPAD`;挑出**老派游戏真正需要**的那几个 |
| 16:30 ~ 21:00 | **★ 链接问题**:`unresolved external symbol`;MSDN 的"平台要求"很可疑 |
| 21:00 ~ 26:00 | **★ 决定自己动态加载**:函数指针 `typedef` + 那个**宏技巧** |
| 26:00 ~ 30:30 | **★ stub + 现场崩溃**:调用未初始化的函数指针 → **执行地址 0** |
| 30:30 ~ 38:00 | **★ `Win32LoadXInput()`**:`LoadLibrary` + `GetProcAddress`;为什么选 **1.3** 而不是 1.4 |
| 38:00 ~ 42:30 | **手柄实测**:A 键驱动画面 + `XInputSetState` 震动(他真跑去拿了个手柄) |
| 42:30 ~ 58:00 | **★ 键盘输入**:`WM_KEYDOWN`/`WM_KEYUP`、VK 码、`LPARAM` 的 bit 30/31、**过滤按键重复** |
| 58:00 ~ 1:32:00 | **Q&A**(含本节课最重要的态度:别过早想底层优化)+ 收尾清理(⚠️ 撤销按值传) |

---

## 二、XInput 是什么,以及**为什么非用它不可**

📖 他把这段历史讲得很清楚:

| 时代 | API | 说明 |
|---|---|---|
| 先 | **DirectInput** | 微软自己搞的"拿手柄输入"的接口层 |
| 后 | **XInput** | 几年后微软**掉头做了个全新的**,DirectInput 基本被放着不管了 |

**两者"不真正兼容"**(他的原话):

- XInput 手柄**能在 DirectInput 里出现**,但是:
  - ❌ **发不了震动(rumble)**
  - ❌ **拿不到音频句柄**(插在手柄上的耳机没法用)
- 所以**想正经支持手柄,基本必须用 XInput** —— 因为"大家手里就是这种手柄"。
- **DirectInput 只在一种情况下还有意义**:支持那些**没有 XInput 驱动的老手柄**。

**好消息**:📖 XInput **是 Windows 上最简单的 API 之一**,基本就是:

```c
for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
{
    XINPUT_STATE State;
    DWORD Result = XInputGetState(ControllerIndex, &State);
    if (Result == ERROR_SUCCESS) { /* 手柄在,State 里有全部信息 */ }
}
```

要点:

| 项 | 说明 |
|---|---|
| `XUSER_MAX_COUNT` | **手柄数量上限**(Xbox 就是 4 个)。⚠️ 他特意**用这个宏**而不是写死 `4` —— 万一以后微软加数量,代码自动跟上 |
| 文档里的坑 | 📖 MSDN 正文写"取值 0 到 3",而宏 `XUSER_MAX_COUNT` 才是权威(他吐槽"文档有点过时") |
| 返回值 | `ERROR_SUCCESS`(0)= 手柄在;**其它值 = 没插/不可用**。他吐槽这个命名:`ERROR_SUCCESS`(错误 成功?) |
| "没插"**不是单纯报错** | 这是**有用的信息** —— 以后可以在界面上提示"手柄已断开",并且**它会被当成一个正常要处理的情况**,而不是异常 |

---

## 三、轮询 vs 中断,以及那个 **packet number**

### 1. 两种拿输入的方式

| 方式 | 含义 |
|---|---|
| **中断式(interrupt)** | 设备**有事才告诉你**(按键、摇杆动了就发数据)。老年代是**真·CPU 中断**;现在基本见不到了 |
| **轮询式(polling)** | **你主动去问**,它才给你状态 |

📖 现在大多数设备是"轮询"或者"设备发包 → 你这边缓冲 → 你去读"—— **形式上有像中断,但其实是你去读一条流**。

**XInput 是轮询式** → 所以**必须自己挑一个时机去问**,他选的是:

```
主循环里:先处理完 Windows 消息  →  然后轮询 XInput  →  然后渲染  →  然后 blit
```

### 2. 🟡 一个"以后可能要调"的疑问

他留了个 TODO:**我们是不是需要比"每帧一次"更频繁地轮询?**
60 次/秒可能完全够,也可能不够 —— 他明确说:**"唯一知道的办法就是试"**,而且这事要等到**真正做平台层的收尾工作**(也就是整个系列很后面)才管。

### 3. `dwPacketNumber`:一个"我是不是问得太慢"的指示器

`XINPUT_STATE` 的第一个字段是 **`dwPacketNumber`**:📖 每当**手柄状态发生变化**,它就 +1。

| 观察到的现象 | 可能的含义 |
|---|---|
| 两次轮询之间,packet number 从 5 跳到 30 | 中间有一堆状态**我们没看到**(按键被按了又松、摇杆抖了) → **可能是轮询得太慢** |
| 但它也可能只是采样率很高,而人根本按不了那么快 | 所以它**只是"信息",不是定论** —— 🟡 他也不知道 XInput 内部是驱动在更频繁地采样,还是手柄自己管的 |

---

## 四、`XINPUT_GAMEPAD` 逐字段(以及**老派游戏需要哪些**)

📖 `wButtons` 是个 **16 位位域(bitfield)** —— 他顺带复习了 Windows 的类型名:

| Windows 类型 | 实际大小 |
|---|---|
| `WORD` | **16 位**无符号 |
| `BYTE` | **8 位** |
| `SHORT` | **16 位有符号** |

| 字段 | 内容 |
|---|---|
| `wButtons`(WORD,位域) | `XINPUT_GAMEPAD_DPAD_UP/DOWN/LEFT/RIGHT`、`START`、`BACK`、`LEFT_THUMB`、`RIGHT_THUMB`、`LEFT_SHOULDER`、`RIGHT_SHOULDER`、`A`、`B`、`X`、`Y` |
| `bLeftTrigger` / `bRightTrigger`(BYTE) | **扳机是模拟量**:0~255,按多深是多少 |
| `sThumbLX` / `sThumbLY` / `sThumbRX` / `sThumbRY`(SHORT) | 左右摇杆的 X/Y(16 位**有符号**) |

> 他顺嘴提了一句:**"intro to C 里有人问位操作多久用一次 —— 我们一直在用,而且光写个 Windows 层就已经用了一大堆。"**

### 他**明确挑出**要用的字段

| 要 ✅ | 不要 ❌ | 为什么 |
|---|---|---|
| **D-pad 四个方向** | —— | 老派游戏的基本方向输入 |
| **Start / Back** | —— | 菜单/暂停之类 |
| **左右肩键**(`SHOULDER`) | —— | 他点了名想要 |
| **A / B / X / Y** | —— | "我们大概主要就用这四个键" |
| **左摇杆 X/Y** | —— | ⚠️ 理由很实际:**Xbox 360 手柄的 D-pad 很差**,所以摇杆可能还是得支持 |
| —— | **扳机(trigger)** | 老派游戏用不上(那时候没有第一人称射击那套) |
| —— | **右摇杆 / 摇杆按下(THUMB)** | "**摇杆按下去对我来说太难按了**",不想要 |

> 他说:这些东西**暂时只是"取出来放在那儿"**,因为我们**还没有任何游戏逻辑可以接**。
> 等真有东西可以接了再接线。

---

## 五、★ 链接问题:为什么不直接链接 `xinput.lib`

按他的流程走一遍就会撞上:

```
unresolved external symbol _XInputGetState referenced in function _WinMain
```

📖 他教的常规做法是:**去 MSDN 那个函数页面底部看"要链接什么"** —— 那里会写 `xinput.lib`。
**但这次有个问题**:

| MSDN 的 "Platform Requirements" 写着 | 他的反应 |
|---|---|
| 要求 Windows 8 | "大多数人没有 Win8,**因为没人想要它**" |
| 要求 DirectX SDK | "哪个版本?用户装了吗?我们其实一个都没用过" |
| 写着 Vista,却没写 Windows 7 | "如果我是在 XP 上跑呢?" |

**所以他不放心直接链接 `xinput.lib`,理由是**:

> [!warning]
> **如果直接链接,程序启动时由 Windows 加载器解析导入表。一旦用户机器上找不到那个 DLL,游戏就"整个加载不起来"(flat out won't load)。**

而这**完全不可接受**,因为:

> **这个游戏你不插手柄也能用键盘玩。** 因为 XInput 缺失就打不开游戏,是荒唐的。

**→ 结论:自己加载。** 不要 Windows 加载器插手。

---

## 六、★ 自己动态加载:三件套 + 那个"宏技巧"

### 1. 第一步:把"函数声明"变成"函数指针类型"

他从 Windows 头文件里**抄函数签名**,然后:

- **去掉 `_In_` / `_Out_` 这些 SAL 注解** —— "那只是给**静态代码分析器**看的,我们不需要"。
- 把它变成**指向该签名函数的指针类型**的 `typedef`:
  - C 里**不能定义"函数对象"**(`x_input_state Fu;` 不合法),
  - 但**可以定义"指向这种函数的指针"**,完全合法。

### 2. 第二步:用**宏**生成签名(他只用这一种写法)

```c
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);          // ← 生成"类型"
X_INPUT_GET_STATE(XInputGetStateStub) { return 0; }    // ← 生成"stub 函数"
```

**这个宏的真正价值**(他在 Q&A 里又强调了一遍):

| 好处 | 说明 |
|---|---|
| **签名只写一处** | 以后签名要改,**改一行就全改**(类型、所有指针、所有 stub 一起变) |
| **以后写 dispatch table 时特别好用** | 一堆**同签名**的函数,想统一换签名就是一行的事 |
| 他说这是他**唯一**写函数指针的方式 | 还在自己的 Windows 开发博客里写过,读者回信说"从没见过这种写法" |

### 3. 第三步:全局函数指针 + **`#define` 重映射**

```c
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_
```

| 问题 | 他的解决 |
|---|---|
| **和 Windows 头文件冲突** —— 头文件里已经声明了真的 `XInputGetState` | 📖 那些**不是宏,没法 `#undef`**;直接同名会冲突。所以**加个尾缀 `_` 区分**,再用 `#define` 把"代码里写的 `XInputGetState`"**重映射到我们自己的指针** |
| 为什么要重映射,不直接改所有调用点 | ① 保留原名字,读起来还是 `XInputGetState(...)`;② **防止有人不小心调到 Windows 那个真函数**(反正没链接,会报链接错,但他想提前拦住) |
| 另一个选项 | 他提过"干脆不 include 那个头文件,自己把常量抄进来" —— 🟡 但他现在不做 |

### 4. ⚠️ 为什么指针要**初始化成 stub**

```c
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
```

如果不初始化:

- 它是 `static` 全局变量 → **默认就是 0**;
- 一旦调用 → **跳到地址 `0x0000000000000000`** → **立刻崩**(他现场演示了);
- **知识点**:📖 **访问违例不只在读/写无效内存时发生,"执行"无效内存也一样** —— 是同一种异常。

**→ 最佳实践:函数指针一开始就指向一个"什么都不做、返回 0"的 stub。** 这样:

> **加载失败也不会有大事** —— 程序照旧跑,只是"手柄功能等于没有"。

---

## 七、`Win32LoadXInput()`:真正把它加载进来

```c
internal void
Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}
```

| API | 说明 |
|---|---|
| **`LoadLibrary`** | 把一个 DLL 映射进我们的进程地址空间。返回 **module handle** —— 📖 "相当于**一段已加载代码的句柄**",和 `WinMain` 里的 `HINSTANCE` 是同一类东西。⚠️ 也有 A/W 版,他后来改成了 **`LoadLibraryA`** |
| **`GetProcAddress`** | 从那个模块里**按名字取函数地址**。⚠️ **它不知道签名**,所以返回 `void *`(实际上是 `FARPROC`)→ **必须强制转换成我们自己的函数指针类型**,否则 C 会抱怨 |
| `GetProcAddress` 没有 A/W 版 | 他现场查了,确认只有一个版本 ✅ |

### ⭐ 为什么加载 **`xinput1_3.dll`** 而不是 1.4

他现场用 `dir /s xinput*.dll` 验证过(Win 机器上通常有**两份 1.3**:`SysWOW64` 里是 32 位、`System32` 里是 64 位 —— 他吐槽"这个命名有点反直觉")。

> **他的原则:加载"我知道能用的、最老的那个版本"。**
> 因为 **①** 老版本在更多机器上本来就有;**②** 如果强行要 1.4,**这台机器上就跑不起来**;
> **③** 他**不喜欢让安装程序往用户机器上塞一堆东西**(VC 运行库、各种 redistributable……"我甚至不知道那一半是干嘛的")。
>
> 如果用户真的没有,就在网页上写一句:"想用手柄又没有的话,去微软官网下个 XInput 装上就行。"

**验证**:他在调试器的 **Watch 窗口**里看到这两个指针指向 **`xinput1_3.dll` 在我们地址空间里的区域** —— 📖 说明加载和取地址都成功了。

### 震动(顺手做的)

```c
XINPUT_VIBRATION Vibration;
Vibration.wLeftMotorSpeed  = ...;   // WORD,0~65535
Vibration.wRightMotorSpeed = ...;
XInputSetState(0, &Vibration);
```

`XINPUT_VIBRATION` 只有两个字段:**左马达速度 / 右马达速度**。他填了个接近最大值,实测**手柄真的震了**(还专门跑去另一个房间拿了个手柄来试)。

---

## 八、★ 键盘输入:`WPARAM` / `LPARAM` 解密

### 1. 用哪几个消息

| 消息 | 说明 |
|---|---|
| **`WM_KEYDOWN`** | 按下 |
| **`WM_KEYUP`** | 抬起 |
| `WM_SYSKEYDOWN` / `WM_SYSKEYUP` | 带 Alt 的系统键(他提了,本次**没处理**) |

📖 **你不需要注册任何东西** —— Windows **自动**把这些消息送进你的窗口过程。
之前它们全都落到 **`DefWindowProc` 被"吃掉"**了,现在改成**自己处理**。

### 2. 参数含义

| 参数 | 含义 |
|---|---|
| **`WPARAM`** | **虚拟键码(VK code)** —— 告诉你**是哪个键** |
| **`LPARAM`** | **一个位域**,关键两位见下 |

📖 **`LPARAM` 的两位**:

| 位 | 名字 | 含义 |
|---|---|---|
| **bit 30** | 之前的按键状态 | 这条消息**之前**,键是按下(1)还是抬起(0) |
| **bit 31** | 转换状态(transition) | **`WM_KEYDOWN` 永远是 0;`WM_KEYUP` 永远是 1** |

他把它"**解密成两个可靠的布尔**":

```c
bool WasDown = ((LParam & (1 << 30)) != 0);
bool IsDown  = ((LParam & (1 << 31)) == 0);
```

> [!warning]
> **为什么要写 `!= 0`**(他专门停下来讲):`LParam & (1 << 30)` 得到的结果**不是 0 或 1**,而是 **0 或者"bit 30 被置上的那个大数"**。
> 而 **C 里的比较运算保证产出 0 或 1**,所以 `!= 0` 正好把它**规范成 0/1**。
> (他顺便承认:intro to C 里讲这条的时候他自己**也有点不确定**,挺奇怪的。)
>
> 💡 他后来在 Q&A 里补充:写成 `#define KEY_MESSAGE_WAS_DOWN_BIT (1 << 30)` 之类**更清楚**的做法也很好,
> 而且 **`编译器不笨,两种写法生成的代码一样**。

### 3. ⭐ 过滤"按键重复(key repeat)"

**现象**:**按住一个键不放**,会**反复收到 `WM_KEYDOWN`**。
此时 `WasDown == IsDown == true`(之前是按下,现在也是按下)—— **这种消息我们不要**。

**过滤办法**:

```c
if (WasDown != IsDown)   // 只有"状态真的变了"才处理
{
    // 真正的按下 或 真正的抬起
}
```

> 他现场**写反了一次**(写成相等),然后自己笑:"我老是这样。" —— 这条和 Day 4 的"XOffset 写在循环里"一样,
> 属于他直播时的经典翻车现场,值得记下来提醒自己。

### 4. VK 码怎么记

| 键 | 写法 |
|---|---|
| **字母 / 数字** | **直接就是 ASCII 大写**:`'W'` `'A'` `'S'` `'D'` `'Q'` `'E'` ✅ 他实测验证过(打 `OutputDebugStringA("W\n")`) |
| 方向键 | `VK_UP` / `VK_DOWN` / `VK_LEFT` / `VK_RIGHT` |
| 空格 | **`VK_SPACE`**(不是 `VK_SPACEBAR`) |
| Esc | **`VK_ESCAPE`**(不是 `VK_ESC`) |

📖 MSDN 上有**完整的 VK 码列表**。

### 5. 他选了什么,以及"这不算代码"

- 挑的键:**W/A/S/D、Q/E**(他说"Q 和 E 是那种一定要有的额外选项"),**方向键、空格、Esc**。
- 用 `if/else 级联` 而不是 `switch` —— 观众问起,他回答:**"这根本不是'选择'。"**
  - 这段代码**只是先把通道摆在那儿**,等真有游戏逻辑了再接线;
  - 以后**爱改成 `switch` 就改**,两种都行,**现在它连"选择"都算不上**。
- 📖 他明确说:**真正"发布版"的平台层会用 `Raw Input`** —— 那样能处理**多个键盘分别对应不同玩家**(分屏),但**复杂得多**,而且"除了想支持多键盘的人,你根本不需要",所以**以后再说**。

---

## 九、⚠️ 结尾的自我更正(这条**直接影响我们的代码**)

### 1. 撤销"按值传 `win32_offscreen_buffer`"

他在 Day 005 为了"演示按值传可能更好",把那个结构体改成了**按值传**。这次他说:

> **"我周末想了想,还问了朋友 —— 我不喜欢这么做。"**

| 他的理由 | 说明 |
|---|---|
| **那不是我会写的代码** | 那么大的结构体按值传,**只有在优化器把它内联(inline)之后**才可能赚到;如果**没内联**,就只是**白白强制拷贝** |
| **我应该坚持我平时的写法** | 想介绍别的写法,可以说一句"你可以自己去研究",但**不该为了'可能更好/更新'就去做自己不熟悉的写法** —— "因为我**没有保证它是对的**,可能反而把大家带偏" |

**→ 他改回了传指针。** 📍**这就是我们代码要跟着改的地方**:见第十一节。

### 2. 其它收尾

| 做了什么 | 说明 |
|---|---|
| 字符串 API 加 **`A`** 后缀 | `LoadLibrary` → **`LoadLibraryA`**;📖 和 Day 002 Q&A 的约定一致(`GetProcAddress` 没有 A/W 版) |
| **没做**(时间不够) | **设置光标**(`SetCursor`)、**全屏** |

---

## 十、Q&A 要点

| 问题 | 他的回答 |
|---|---|
| **`LPARAM` bit 30 对 `WM_KEYUP` 永远是 1 吗?** | 是的。但**我们不在乎** —— 我们要的是把**四种**按键消息统一"解密"成两个可靠的东西:**之前是不是按下**、**现在是不是按下** |
| **为什么变量名首字母大写?** | 习惯,来自**他最早和 Chris Hecker 一起编程**的时候。**没有对错** |
| **结构体多大开始"按值传"会有问题?** | **不知道,得实测**。`RECT` 是 4 个 32 位值 = **128 位**,和"传两个指针"一样大,按值传完全没问题;大了就别冒险,**传指针** |
| **★ 一位观众建议某行"这样写更省事"** | **那样更晦涩。** 写得让人一眼看懂更重要,而且**这种简单代码编译器两种写法生成的都一样** |
| **★ 最重要的态度(他专门讲了很长一段)** | **「在你真正要优化的时候,再去想底层优化。」** 他承认**上周可能把大家带偏了**:如果一边打字一边想"这样写会不会更快",**你很可能在浪费时间** —— 底层优化非常难、非常具体、因素互相牵连,**你很可能一直猜错**,甚至"以为在帮忙、其实在伤害"。正确做法:**① 写得干净简单**(这才好优化);**② 真要优化,就去看生成的汇编、实测确认** |
| **`LoadLibrary` 怎么找到用户的 xinput?** | 微软有专门文档 **"Dynamic-Link Library Search Order"**。📖 **第一站就是我们的 exe 所在目录** → 所以理论上可以把 DLL 放 exe 旁边;但他**通常不这么做**(不想让安装程序往用户机器上装东西),而是**加载能找到的最老版本** |
| **能不能在编译期链接 XInput?** | **不行那只是个 import library(导入库)**,它做的事**和我们手写的动态加载一模一样**:加载时找地址、把指针 patch 进 exe。区别只是**Windows 加载器替我们做了**。`GetClientRect` 这些也是这样 |
| **stub 在没装 XInput 的 XP 上真能工作吗?** | **唯一办法就是测**。所以他**在虚拟机里装了一个正版 Windows XP** 准备验证(本来想留到最后当彩蛋)。他还提前说:**现在肯定跑不了 XP** —— 微软编译器默认产出的 exe 在 XP 上跑不起来,得加特殊链接器开关(以后演示) |
| **检查 `GetProcAddress` 返回 0 是浪费时间吗?** | **有点。** 他真实代码里确实会检查(失败就把指针指回 stub),但**防这个基本没意义**:如果 DLL 都能被换掉、函数都找不到,那函数本身能不能用更值得怀疑 |
| **Raw Input 对性能没影响?** | **不同意**,他认为影响不小,这也是他**不用它**的原因之一 |
| **`WPARAM`/`LPARAM` 从哪来?** | 从 **`user32.dll`**(调用栈上就在我们上面)。之所以叫 W/L 而不是具体类型,是因为**它要用同一个函数回调所有消息**,而每条消息参数类型都不同 → 只能塞两个**不透明参数**,由你按 message 去"解密"(强转或位运算)。他现场查了:**64 位下它们都是 64 位** |
| **`WPARAM`/`LPARAM` 在 XP 上是 32 位?** | 那不关 XP 的事,**是 32/64 位编译模式决定的**;我们现在是 64 位 |
| **会不会再用 `Paint` 里的 width/height?** | **不会,已经删掉了。** 现在只传**窗口宽高** |
| **以后用 DirectX 还是 OpenGL?** | 可能用其中一个来做**混合**(为了速度)。**OpenGL 在 Windows 上有时不太好用**,所以可能 **Windows 用 DirectX、别的平台用 OpenGL** |
| **Valgrind 为什么把"没释放"报得那么吓人?** | 它是外部工具,**分不清"故意交给 OS 清理"和"真泄漏"**。程序开头分配一块最后不释放**完全合法**;**每帧分配不释放才是真泄漏**。它只能全报,得你自己判断 |
| **★ 能不能在拖动窗口时保持帧率稳定?** | 他说注意到**很多人在给这个代码测帧率**,"**强烈建议不要**":<br>① 现在只是**用一个大概率在慢路径上的 Windows 调用**把东西贴上屏幕;<br>② **这不是"快 blit"的方案**,只是"能用简单方式把自绘位图弄上屏幕";<br>③ **而且最后压根不会用 `StretchDIBits`**;<br>④ "**别在马还没学会走之前就想让它跑**" —— 等真做完渲染器再谈"怎么最快送上去" |
| 明天做什么? | **DirectSound**(开始做声音) |
| 作息 | 这周三是 8pm 的最后一次;**感恩节周三四五改成上午 11 点** |

---

## 十一、和我们代码的关系 / 下一步(重要)

> 我们现在:`D:\HandMade\HandMade\code\win32_handmade.cpp`,**358 行**,编译 ✅ 0 error / 0 warning。
> 已经完成:结构体化、`Win32GetWindowDimension`、`Win32DisplayBufferInWindow` 收窗口宽高、**固定 1280×720**、`WM_SIZE` 不再重建。

### 11.1 ⚠️ **Day 006 推翻了我们的"按值传"(必须改)**

| 我们的现状 | Day 006 的结论 | 要不要改 |
|---|---|---|
| `RenderWeirdGradient(win32_offscreen_buffer Buffer, ...)` —— **按值传** | Casey **撤销**了按值传,理由是"结构体太大,不内联就只是白白拷贝",而且"这不是我平时的写法" | ✅ **改回传指针**:`(win32_offscreen_buffer *Buffer, ...)` |
| `Win32DisplayBufferInWindow(..., win32_offscreen_buffer Buffer, ...)` —— **按值传** | 同上 | ✅ **改回传指针** |
| `Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, ...)` —— 传指针 | ✅ 本来就对 | ⬜ 不动 |

> [!warning]
> 这是**第二次**出现"Day 5 说按值、Day 6 说传指针"的反转,**所以别把 Day 005 文档里"按值传"那条当定论** ——
> 本文件这一节才是最新的。📍**说明**:Day 005 里他当场也承认过"我不知道哪个更好,我们试试看",这次他给出了结论。

### 11.2 其它要跟着改的

| 项 | 说明 |
|---|---|
| **删掉 `Win32DisplayBufferInWindow` 的 `X/Y/Width/Height`** | Day 006 Q&A 里他说**"已经删掉了,现在只传窗口宽高"** ✅ 正好和 Day 005 的"别管脏区域"结论一致 |
| **`Paint.rcPaint` 那 4 行** | 随之删掉(`WM_PAINT` 里只剩 `BeginPaint`/`EndPaint`) |
| 字符串 API 加 **`A`** 后缀 | 我们还没用到 `LoadLibrary`;以后加了要写 `LoadLibraryA` |
| **别过早想底层优化** | 这条对我们**很有用**:不用急着抠"哪种写法更快",先写干净 |

### 11.3 Day 006 新增的功能(我们**还没有**)

| 要做的 | 关键点 |
|---|---|
| **`Win32LoadXInput()`** | `LoadLibraryA("xinput1_3.dll")` + `GetProcAddress`;两个函数指针**初值指向 stub** |
| **`XInputGetState` 轮询** | 放在**消息循环之后、渲染之前**,每帧一次,循环 `XUSER_MAX_COUNT` 个手柄 |
| **输入结构体** | 他这次只是把值取出来打印/试;真正的做法是**打包成一个 `game_input` 结构体**传给游戏层(系列后面几集会做) |
| **键盘** | `WM_KEYDOWN`/`WM_KEYUP` 两个 case;`WasDown`/`IsDown` 解密 + `if (WasDown != IsDown)` 过滤重复 |
| **按 `xinput.lib` 的诱惑要顶住** | ⚠️ **不要**为了省事去 `#pragma comment(lib, "xinput.lib")` —— 那会让"没装 XInput 的机器直接打不开游戏" |
| **`#include <xinput.h>` 是安全的** | 头文件里只有声明和常量;冲突用 `#define` 重映射解决(见第六节) |

### 11.4 我们**现在代码里已有的小问题**(顺带提醒,Day 006 没提但仍然是问题)

| 项 | 说明 |
|---|---|
| 循环外那个 `HDC DeviceContext = GetDC(Window);` | **借了不还 + 是死变量**(被循环内同名变量遮蔽)→ 删掉 |
| `RenderWeirdGradient` 没有空指针守卫 | `VirtualAlloc` 失败会写地址 0 崩;**加个 stub 式守卫**(和 Day 006 的"指针初始化为 stub"是同一个思想) |
| `OutputDebugStringA("WM_SiZE\n")` 拼写错 | 改 `WM_SIZE` |

---

## 附录 A:本集新出现的 API / 常量速查

| 名字 | 作用 | 来自 |
|---|---|---|
| `XInputGetState` | 拉取某个手柄的状态 | xinput1_3.dll |
| `XInputSetState` | 发震动 | xinput1_3.dll |
| `XINPUT_STATE` | 状态结构体,首字段是 `dwPacketNumber` | xinput.h |
| `XINPUT_GAMEPAD` | 手柄细节(`wButtons` 位域 / 扳机 / 摇杆) | xinput.h |
| `XINPUT_VIBRATION` | 左右马达速度 | xinput.h |
| `XUSER_MAX_COUNT` | 手柄数量上限(4) | xinput.h |
| `ERROR_SUCCESS` | 0 = 手柄存在 | winerror.h |
| `LoadLibrary` / `LoadLibraryA` | 把 DLL 映射进进程,返回 module handle | kernel32 |
| `GetProcAddress` | 按名字取函数地址(返回 `void*`,**要强转**) | kernel32 |
| `HMODULE` | 模块句柄(和 `HINSTANCE` 同类) | winnt.h |
| `FARPROC` | `GetProcAddress` 的返回类型 | winnt.h |
| `WM_KEYDOWN` / `WM_KEYUP` | 键盘按下 / 抬起 | winuser.h |
| `WM_SYSKEYDOWN` / `WM_SYSKEYUP` | 带 Alt 的系统键(本次没处理) | winuser.h |
| `VK_UP/DOWN/LEFT/RIGHT`、`VK_SPACE`、`VK_ESCAPE` | 虚拟键码;字母数字直接写 `'W'` | winuser.h |
| `Raw Input` | 📖 以后"发布版"平台层用它支持多键盘(本次不做) | —— |
| `Dynamic-Link Library Search Order` | 📖 微软讲 `LoadLibrary` 搜索顺序的那篇文档 | MSDN |

---

## 附录 B:本集值得抄下来的三句话

1. **"如果用户机器上找不到那个 DLL,游戏就整个加载不起来 —— 而这个游戏不插手柄也能用键盘玩。"**
   → 所以**宁可自己写 `LoadLibrary` + `GetProcAddress`,也不链接 `xinput.lib`**。
2. **"函数指针一开始就指向一个什么都不做的 stub"** —— 加载失败也不会有大事。
3. **"在你真正要优化的时候,再去想底层优化。"** —— 他自认上周"按值传"的演示**把大家带偏了**。

---

## 附录 C:下周(他亲口说的)

| 时间 | 内容 |
|---|---|
| 周二(次日) | 开始 **DirectSound**(做声音) |
| 本周剩余 | 声音;如果三天搞定,周五做**文件 IO** |
| 之后 | "那就基本准备好**下下周开始做真正的游戏**了" |
| 感恩节 | 周三 8pm 是本周最后一次;**周三四五改成上午 11:00** |

---

## 附录 D:打字过程中我问过的问题 + 答案(2026-09-23 ~ 09-24)

> 这一节是**我照这一集打字时问 AI 的问题**,按提问顺序记。
> 原始问答在会话里;这里是**结论 + 证据**,方便以后回查。凡标 🟡 的都是推断。

### D.1 `typedef DWORD WINAPI x_input_get_state(...)` 是什么意思?

**它定义的是"函数类型",不是函数、也不是变量** —— 一张"这种函数长什么样"的规格单:

| 部分 | 含义 |
|---|---|
| `DWORD` | 返回类型 |
| `WINAPI` | 调用约定 = `__stdcall`(参数怎么压栈、谁清栈、名字怎么修饰) |
| `x_input_get_state` | **类型名**(不是函数名) |

⭐ 所以下面必须写 `x_input_get_state *` 才是指针 —— **C 里"函数类型"和"函数指针类型"是两个东西**。

- 它和第六节那个宏 `#define X_INPUT_GET_STATE(name) DWORD WINAPI name(...)` **完全等价**,宏只是省得把签名写两遍。
- 和 SDK 头文件对照(`Xinput.h` 里是):
  ```c
  DWORD WINAPI XInputGetState(_In_ DWORD dwUserIndex, _Out_ XINPUT_STATE* pState) WIN_NOEXCEPT;
  ```
  我们只是**把函数名换成类型名**、去掉 `WIN_NOEXCEPT`。
  🟡 `WIN_NOEXCEPT` 在 `winnt.h` 里是 `#define WIN_NOEXCEPT noexcept`,而 C++17 起 `noexcept` 属于**函数类型的一部分** ⇒ 严格说我们的类型和 SDK 的不完全相同。无害:我们只用它做 `GetProcAddress` 的强转、给自己 stub 赋值,从不混用。
- `_In_` / `_Out_` 是 **SAL 注解**,**不生成任何代码**(给静态分析器看);`_Out_` 表示"函数往里写"。证据:`cl -E` 之后它们**整片消失**。

### D.2 那两个 `global_variable x_input_get_state* ...;` 有什么用?

**把"调用哪个函数"从编译期推迟到运行期。**

| | 直接调 `XInputGetState` | 走我们自己的指针 |
|---|---|---|
| 链接期 | 必须有导入库,否则 `LNK2019`(我们卡在这里过) | 完全不提那个符号 |
| 启动期 | Windows 要求 `xinput1_3.dll` 存在,否则程序**打不开** | 我们自己 `LoadLibraryA`;没有 DLL 就退回 stub |
| 调用时 | 直接跳导入表 | 读指针里的地址,再跳过去 |

一句话比喻:**typedef 是"函数形状的模具",两个 `global_variable` 是"两个可以插不同实现的插座"。**

⚠️ 但光有这两行不够,还缺三样:① **填地址**(`LoadLibraryA` + `GetProcAddress`)② **先插 stub**(不填就调用 = 跳地址 0 崩)③ **调用点走插座**(或用 `#define` 重定向,见 D.3)。

### D.3 宏那一段每一步到底干了什么?(预处理器证据)

用 `cl -E` 导出的**真实展开**(不是回忆):

| 源文件里写的 | 预处理之后 | 含义 |
|---|---|---|
| `typedef X_INPUT_GET_STATE(x_input_get_state);` | `typedef DWORD __stdcall x_input_get_state( DWORD dwUserIndex, XINPUT_STATE* pState);` | 造出**函数类型** |
| `X_INPUT_GET_STATE(XInputGetStateStub) { ... }` | `DWORD __stdcall XInputGetStateStub(...) { ... }` | **定义一个真函数**(stub) |
| `global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;` | `static x_input_get_state* XInputGetState_ = XInputGetStateStub;` | 全局函数指针,**初值指向 stub** |
| `dwResult = XInputGetState(...);` | `dwResult = XInputGetState_(...);` | `#define` 把调用点**文本替换**成走指针 |

三条副产品事实:`_In_`/`_Out_` **消失**;`WINAPI` → `__stdcall`;`global_variable` → `static`。
📍 那次预处理的证据文件在 `HandMade\build\pp.i`(318,185 行;已删,想复现见下方命令)。

⭐ 用宏的**真正理由**:同一个宏既生成 typedef、又生成 stub 的函数头 ⇒ **两处签名永远一致**。
手写两份也能编过,但签名会漂移 —— 而 `GetProcAddress` 返回的是**无类型指针**,这类错**编译器不会替你发现**。

⚠️ 当时发现一个真 bug:stub 写的是 `return 0;`,而 `winerror.h` 里 **`ERROR_SUCCESS = 0`**、`ERROR_DEVICE_NOT_CONNECTED = 1167`
⇒ 调用点 `if (dwResult == ERROR_SUCCESS)` 会**永远成立**,程序认为"手柄永远插着"。**stub 应该说"我没有设备"。**

```bat
:: 复现宏展开(证据怎么做出来的)
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo
cl -E -nologo -MTd -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1 code\win32_handmade.cpp > build\pp.i
:: 然后在 pp.i 里搜 x_input_get_state / XInputGetState_
```

### D.4 这套技术叫什么?(想自己搜的话)

| 层次 | 名字 |
|---|---|
| **机制** | **Run-Time Dynamic Linking**(运行时动态链接 / 显式链接);对照 **Load-Time Dynamic Linking**(隐式链接,靠 `.lib` 导入库) |
| **技巧** | **Stub(桩函数)** + "function pointer initialized to a stub" |
| **模式** | Function-pointer dispatch table / late binding / backend abstraction |
| 跨平台对照 | POSIX 的 `dlopen` / `dlsym` |
| 现实同款 | GLEW / glad(OpenGL 扩展加载)、`SDL_LoadObject`、Vulkan 的 `vkGetInstanceProcAddr` |
| ⚠️ 别混淆 | **Delay Load**(`/DELAYLOAD`,链接器自动做)、trampoline / thunk / hook、**X-Macro** |

**搜索词**:`run-time dynamic linking LoadLibrary GetProcAddress` / `explicit vs implicit linking DLL import library` /
`function pointer dispatch table late binding` / `dlopen dlsym`。

**Casey 自己的说法**(字幕是 ASR 文本,给行号/时间戳便于回查):

| 时间 | 他说什么 |
|---|---|
| `00:28:48` | "…want to actually do an XInputGetState **stub**" |
| `00:30:37` | "a good way to **initialize your function pointers to just point at a stub**" |
| `01:02:41` | "Define the stubs by using the macro just works really well" |
| `01:24:21` | "import library approach … rather than **true dynamic loading**" |

### D.5 `WPARAM` 和 `LPARAM` 有什么区别?

**名字是历史残留**:W = Word(16 位)、L = Long(32 位)、PARAM = 参数 ⇒ **Win16 时两者尺寸真的不同**;
Win32 都变 32 位、Win64 都变 64 位 —— **今天尺寸一样**。

真实定义(从 SDK 头文件里挖的):

```c
minwindef.h:186   typedef UINT_PTR   WPARAM;   /* basetsd.h: unsigned __int64(x64) */
minwindef.h:187   typedef LONG_PTR   LPARAM;   /* basetsd.h: __int64(x64)          */
```

⇒ **今天唯一剩下的实质差别:一个无符号、一个有符号。其余全靠"每条消息自己规定"。**

| 消息 | WPARAM | LPARAM |
|---|---|---|
| `WM_KEYDOWN`/`WM_KEYUP` | **虚拟键码** | **位域**:bit0-15 重复次数、16-23 扫描码、24 扩展键、29 Alt、**30 之前是否按下**、**31 转换位** |
| `WM_SIZE` | `SIZE_*` 标志 | `LOWORD` = 宽、`HIWORD` = 高 |
| `WM_MOUSEMOVE` | `MK_*` 按键状态 | `LOWORD` = x、`HIWORD` = y(**有符号**) |
| `WM_COMMAND` | `LOWORD` = 控件 id、`HIWORD` = 通知码 | 子窗口 `HWND` |
| `WM_PAINT` / `WM_CLOSE` / `WM_DESTROY` | 都不用 | 都不用 |

> 📖 历史习惯:**大东西(指针、句柄)走 LPARAM** —— Win16 的 32 位远指针塞不进 16 位的 WPARAM。
> 所以 `WM_COMMAND` 把 `HWND` 放在 LPARAM 里,这个习惯活到今天。

**我们踩的两个坑(都是 `-W4 -WX` 下直接编不过)**:

| 写法 | 报什么 | 为什么 |
|---|---|---|
| `uint32 VKCode = WParam;` | **C4244**(可能丢失数据)→ C2220(警告视为错误) | x64 下 WPARAM 是 64 位 ⇒ 要写 `(uint32)WParam` |
| `VKCode == "W"` | **C2446 / C4130** | `"W"` 是**字符串字面量**(类型 `const char[2]`,一个地址);`'W'` 才是**整数 87** |

### D.6 为什么键盘和手柄要分开处理?

**不是 Casey 的选择,是 API 现实。**

| | 键盘 | 手柄 |
|---|---|---|
| 模型 | **消息驱动**(系统推给你) | **轮询**(你去拉) |
| 有没有焦点概念 | **有**(没焦点收不到) | **没有**(窗口在后台也读得到) |
| 有没有消息 | `WM_KEYDOWN` / `WM_KEYUP` 等 | **根本没有** —— XInput 不在窗口系统里 |

Casey 原话(字幕第 **155** 行):"the XInput is a **polling based API** which basically means that it only gives us back the state…"

**更本质的差别:边沿 vs 电平**
- 消息自带**边沿**:`LPARAM` 的 bit30/31 直接告诉你"这是刚按下、还是按住重复" ⇒ **不用记上一帧**;
- 轮询只有**电平**(每帧一张快照)。要"这一帧刚按下"的语义 ⇒ **必须自己保存上一帧做比较**。

**为什么不干脆也轮询键盘**(`GetAsyncKeyState`):
① 会**丢事件**(两帧之间"按下又抬起"根本看不到);② 拿不到重复次数/扫描码那套位域;
🟡 ③ 还有一条"它读全局键状态、不看焦点"——这条我**没现查文档**。

**最终是要合起来的**:平台层吸收差异 ⇒ 归一成 `game_input`(键盘被当成"controller 0"),游戏层只问"这个按钮这一帧按下没有"。
🟡 那是系列后面几集的做法;本集只是把两边**各自**打通。

📍 在我们代码里:键盘在**窗口过程**(`WindowMainWindowCallback`),手柄在**主循环**、`PeekMessage` 排空之后、渲染之前。

### D.7 手柄也有"VKCode",为什么不直接合并?

手柄**没有** `VKCode`。两边只是"看着都是常量",形态完全不同:

| | 键盘 | 手柄 |
|---|---|---|
| 常量 | `VK_*` | `XINPUT_GAMEPAD_*` |
| 形态 | **互斥编号**(一次事件 = 一个键) | **位掩码**(一次给你十六个键的状态) |
| 有没有"是谁的" | 没有(键盘只有一块) | **有**(`XUSER_MAX_COUNT` = 4 个槽) |
| 有没有连续量 | 没有 | **有**:摇杆 `-32768~32767`、扳机 `0~255` |

**直接合并会撞车**(头文件实测):

| 手柄 | 键盘 |
|---|---|
| `XINPUT_GAMEPAD_DPAD_UP` = `0x0001` | `VK_LBUTTON`(鼠标左键)= `0x01` |
| `XINPUT_GAMEPAD_DPAD_DOWN` = `0x0002` | `VK_RBUTTON`(鼠标右键)= `0x02` |

`VK_*` 空间里 `0x01~0x06` 已被鼠标键占用,而手柄的 dpad / start / 摇杆按下正好落在 `0x0001~0x0080` 区间 ⇒ **一套编号放不下两套语义**。

**正确做法:在"动作语义"层合并,不在"设备码"层合并。** `VK_*` 和 `XINPUT_GAMEPAD_*` 只活在**平台层**,一步都不外传。

### D.8 `inline`(内联)是什么?

**把函数的代码直接塞进调用点**,省掉 `call`/`ret` 和参数搬运,并让编译器看到更多上下文。

⚠️ **两个"inline"别搞混**:
- **内联展开** = 一项**编译器优化**,由优化等级和它的成本模型决定;
- **`inline` 关键字** = **链接规则**(允许同一份定义出现在多个 `.cpp`/头文件而不违反 ODR),对"是否真的内联"**只是提示**。

⇒ "写 `inline` 就会变快"是**误解**。MSVC 另有 `__forceinline` / `__declspec(noinline)`,开关是 `/Ob0 /Ob1 /Ob2`(`/Od` 隐含 `/Ob0`)。

**实测(本机 + 我们的参数)**,一个按值收结构体、被调用三次的例子:

| 优化等级 | `Caller` 里实际看到什么 |
|---|---|
| **`-Od`**(我们 `build.bat` 用的) | **三次 `rep movsb`(每次 40 字节)+ 三次 `call`** —— **连 `__forceinline` 都没被内联** |
| `-O2` | **一个 `call` 都没有、一个 `rep movsb` 都没有** —— 全内联,连"拷贝"这个动作都消失了 |

⇒ 这正好解释 Casey 为什么撤销"按值传"(第九节):**那是在赌优化器会内联**。
而我们用 `-Od` ⇒ 按值传 100% 是真拷贝,现在纠结这个**只有语义意义、没有性能意义**。

**代价**:代码膨胀(i-cache)、调试时栈里**看不到被内联的那一帧**。

### D.9 顺带问到的:引擎分几层?什么时候能导入 3D 模型?

- **Casey 只切两刀**:平台层(只有它能碰操作系统)vs 游戏层(不知道 Win32 存在),中间靠**函数表 + 几个结构**通信。
- 工业界常见分层(从下往上):平台/OS 抽象 → 核心(内存/数学/容器) → 输入 → 音频 → 渲染 → 资源 → 网络 → 动画 → 物理 → 表现 → 世界/场景 → 玩法 → 工具。
- ⚠️ **"导入 3D 模型"不是 Handmade Hero 的里程碑**:它**不做**通用模型导入(没有 FBX/OBJ/glTF 解析),
  资产是它自己的 **HHA/HHT** 格式,几何最多做到**立方体 UV**。
- 3D 相关的最早集数(🟡 来自 `explain\HANDMADE_HERO_终点与导航.md`,那份文档自认是推断):
  **071–111** 3D 化 + 透视投影;**112–131** SIMD 光栅化;**331–378** 深度缓冲 / 3D 旋转矩阵;**470–545** 立方体 UV。

> [!warning]
> **而且我们的游戏是类银河城,不是他的地牢**(2026-09-24 定的事):
> **引擎层跟着学;玩法、关卡与地图结构、能力解锁与回溯、存档点、Boss —— 自己设计。**
> 别把 HH 后期的地牢生成、体积光照、Brains 当成"我们也要做"。详见 `README.md` 开头与 `NOTES.md` 的"项目定位"。

### D.10 这一节之后,代码的实际状态(2026-09-24)

上面这些结论**都落到代码里了**,所以**第 11 节那几处"我们还没有"已经过时**:

| 项 | 状态 |
|---|---|
| `#include <xinput.h>`、宏 + typedef + stub + 函数指针 + `#define` | ✅ |
| `Win32LoadXinput()`(`LoadLibraryA` + `GetProcAddress`) | ✅ |
| **键盘**:四个 case 标签共用一块、键位判断挪进 `if (WasDown != IsDown)`、每个键打印 | ✅ |
| `build.bat` | ✅ `[build] OK`(0 error 0 warning) |
| stub 的 `return 0`(应是 `ERROR_DEVICE_NOT_CONNECTED`) | ❌ 还没改(见 D.3 末尾) |
| `Win32LoadXinput()` 取不到导出函数时**退回 stub**的兜底 | ❌ 还没加 |

**键盘是怎么验证的**(不是"编过就算"):
挂到 `cdb` 里跑(它会把 `OutputDebugStringA` 打到控制台)⇒ 抓到 `W` / `A` / `Escape was down`;
再**伪造一条"重复"消息**(bit30=1、bit31=0)⇒ **没有任何输出** ⇒ **重复过滤真的生效**;
另外启动日志里的 `ModLoad: C:\WINDOWS\SYSTEM32\xinput1_3.dll` 顺带证明**动态加载成功**(DLL 是运行时加载的,不是链接进来的)。

> [!warning]
> 补一句:那次验证里,"伪造消息"是 `PostMessage` 直接投递的 —— 它绕过系统输入队列,但走的是**同一个窗口过程**,
> 所以能证明处理逻辑;真实按键那条路径另外用 `keybd_event` 单独验过。
