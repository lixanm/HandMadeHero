# Day 007 - Initializing DirectSound

> **这一集讲了什么**:开场三处更正(stub 返回值 / XInput 1.3↔1.4 / **Alt+F4**)、现场一个**运算符优先级翻车**、
> 以及 DirectSound 初始化的**绝大部分**(主缓冲 + 格式 + 次缓冲)。
> 结果:一路写到最后**两个调用都返回 `E_INVALIDARG`**,直播弹幕抓到原因 —— 是 `WAVEFORMATEX` 的**字段顺序**坑;改完当场通过。
>
> ⚠️ **本文档的证据等级**(沿用 `NOTES.md` §2.2 的规矩):
> - 引用视频原话都标了**时间戳**;字幕是 **ASR 机器识别**,人名/API 名/数字几乎一定会错(本集把 `dsound.dll` 认成 "dound.dll"、`pUnkOuter` 认成 "punk outer");
> - 凡涉及**API 签名、常量值、字段顺序**的,都在 **SDK 头文件**里核实过(下面都给文件名与原文);
> - 文中 **✅ 实测** = 我在你这台机器上真跑过;**🟡** = 推断。

---

## 一、这一集干了什么(总览)

| 段落 | 内容 | 时间 |
|---|---|---|
| 1 | 邮件更正一:stub 应该返回 `ERROR_DEVICE_NOT_CONNECTED`,不是 `0` | `00:00:56` 起 |
| 2 | 邮件更正二:**XInput 1.3 ↔ 1.4** —— Win7 只有 1.3、Win8 只有 1.4 ⇒ 改成**分层加载** | `00:02:23` 起 |
| 3 | 邮件更正三:**Alt+F4 现在得自己处理**(因为我们接管了 `WM_SYSKEY*`) | `00:03:44` 起 |
| 4 | ★ 现场翻车:**运算符优先级**,`LParam & (1 << 30) != 0` 少了一层括号 | `00:06:09` 起 |
| 5 | 顺带讲 `bool` 的性能问题 + 他自己的 `bool32` 习惯 | `00:09:17` 起 |
| 6 | XInput 的一个**性能陷阱**:没插手柄时 `XInputGetState` 会 stall | `00:13:54` 起 |
| 7 | DirectSound 整体设计(环形缓冲 / 48kHz / 每帧 800 样本 / 2 秒缓冲) | `00:16:35` 起 |
| 8 | 真正动手:`LoadLibrary` → `DirectSoundCreate` → 协作级别 → 主缓冲 → 设置格式 → 次缓冲 | `00:23:55` 起 |
| 9 | ❌ 两个调用都 `E_INVALIDARG`;弹幕抓到 `WAVEFORMATEX` 顺序问题;**改完通过** | `00:57:17` / `01:04:06` |
| 10 | Q&A:双缓冲之惑、掉帧爆音、多声道、XAudio、Linux、引擎之争 | `01:03:47` 起 |

---

## 二、开场三处更正

### 1. stub 要返回"设备未连接",不是 0

> `00:01:17` "what we probably want to do here is actually return the error code that says that the devices aren't connected, because right now we're returning zero"

- 他的原话很关键:`zero is my kind of error code return, not Windows` —— **他习惯 0 = 失败,而 Windows 是 0 = 成功**。
- `00:01:47` 他当场翻了 `winerror.h`:**`ERROR_SUCCESS` 就是 0**。
- 所以 stub 应该 `return(ERROR_DEVICE_NOT_CONNECTED);`(✅ 头文件实测值 **1167**),这样"库没加载 = 没有手柄"在调用方看来是一致的。

> 📌 **这条我们早就记过**:`explain\2026-9\2026-9-23.md` §3.1 和 `Day 006` 附录 D.3 末尾都标了"stub `return 0` 语义反了"。
> 现在有 Casey 本人的确认:**不是风格问题,是 bug**。

### 2. XInput 1.3 / 1.4:改成"分层加载"

> `00:02:49` "this call will actually fail on Windows 8, because it does not have XInput 1.3. It has XInput 1.4 only. So basically 7 is likely to only have 1.3, 8 is likely to only have 1.4"

- **做法**:先试 **1.4**(更新的),不行再试 **1.3** —— 两个都失败才退回 stub。
- ✅ **实测(这台机器)**:`XInput1_4.dll`、`XInput1_3.dll`、`xinput9_1_0.dll` **三个都在** `C:\Windows\System32\`。
  也就是说我们**现在**用 1.3 不会出问题,但照 Day 007 改成 1.4 优先更稳(而且这才是他后面的写法)。
- 他还挂了一个 TODO:"哪天在 Windows 8 上测一下"。

### 3. ★ Alt+F4 现在要自己处理

> `00:03:44` "SYSKEYUP and SYSKEYDOWN, those are actually where the Alt+F4 for closing the window comes in"

**机制说得很清楚**:

- Alt+F4 以前是 **`DefWindowProc` 替你干的**;
- 一旦你的窗口过程**开始处理 `WM_SYSKEYDOWN` / `WM_SYSKEYUP` 并且 `break`**,消息就不再落到 `DefWindowProc` ⇒ **Alt+F4 失效**(点关闭按钮还能关)。
- 他的选择:**不**把消息转发回 `DefWindowProc`(那会连带处理其它系统键),而是**自己实现 Alt+F4**:

```c
bool AltKeyWasDown = ((LParam & (1 << 29)) != 0);
if (VKCode == VK_F4 && AltKeyWasDown)
{
    Running = false;      /* 全局运行标志 */
}
```

- `LPARAM` 的 **bit 29 = context code**,文档原话:"**1 if the Alt key is down**"。
- 他解释为什么不干脆转发:

> `01:29:03` 附近 / `00:03:44` 段:"we don't really want it to do any other sys keys… especially because we may not want a cold exit on this"

> ⚠️ **这条正好回答了我们上一轮的遗留问题**。我在给你写键盘那段时标过:"合并四个 case 标签之后 `WM_SYSKEYDOWN` 处理完就 `break`,**Alt+F4 可能不再能关窗**(未验证)"。
> 现在 Casey 亲口确认了这个副作用,并给出解法 —— 见第十四节第 3 条。

---

## 三、★ 现场翻车:运算符优先级(本集最值钱的一段)

### 他写错了什么

```c
bool AltKeyWasDown = (LParam & (1 << 30) != 0);      /* ❌ 少了一层括号 */
```

他本想写:

```c
bool AltKeyWasDown = ((LParam & (1 << 30)) != 0);    /* ✅ */
```

### 为什么结果完全不一样

C 的运算符优先级里,**`!=`(相等类)比 `&`(按位与)高**,所以第一行被解析成:

```c
LParam & ((1 << 30) != 0)   →   LParam & 1
```

`(1 << 30) != 0` 是**真(1)**,于是整个表达式变成 **`LParam & 1`** —— 取的是 **`LPARAM` 的最低位**,
而最低位正是 **`LPARAM` bit0-15 "重复次数"的最低一位**(普通按下时它 = 1)⇒ **恒为真**。

### ✅ 我做的实验(可以复现)

```c
int64 LParam = 1;                                  /* 真 WM_KEYDOWN:重复次数=1,bit30=0 */
int WrongParse = (int)(LParam & (1 << 30) != 0);    /* 他写的 */
int RightParse = (int)((LParam & (1 << 30)) != 0);  /* 他想的 */
```

结果:

```
LParam                        = 0x1
他写的  LParam & (1<<30) != 0 = 1      ← 假阳性:以为 Alt 按着
他想的 (LParam & (1<<30))!= 0 = 0      ← 正确
--- bit30 置上(说明之前就是按下的)---
他写的 = 1
他想的 = 1
```

⇒ **bit30 为 0 时他误判为真;bit30 为 1 时两者都对** —— 所以他打 F4 时"窗口没关",而且看起来毫无规律。

### ⚠️ 更值得记的一条:MSVC 其实会警告,而且在我们这儿是**错误**

✅ 实测(`cl -W4`,同一套参数):

```
precedence_test.cpp(18): warning C4554: "&": 检查运算符优先级是否存在的可能的错误；使用括号阐明优先级
```

**`C4554` 不在我们 `build.bat` 的 `-wd` 抑制列表里** ⇒ 在 `-W4 -WX` 下**它会直接变成编译错误**。
也就是说:**这个 bug 在"手敲学习版"的严格参数下根本活不下来** —— 这是 `-W4 -WX` 值回票价的一个例子。

> 教训(他也在 Q&A 里说了):**位运算和比较运算混用时,永远加括号**。
> 顺带:他 Q&A 里补充"文档那种绕的措辞"也让他怀疑过文档写错了,结果是自己括号掉了。

---

## 四、`bool` 的性能问题,和他的 `bool32`

> `00:09:29` "usually what I do is I don't use bool… because bool is defined in C++ to have some kind of odd semantics"

- 现象:`bool WasDown = (LParam & (1 << 30));` 在**开优化**编译时会出现性能警告
  (**C4800: forcing value to bool 'true' or 'false', performance warning**)。
  原因:`LParam & (1<<30)` 的结果是 **0 或者 0x40000000**,不是 0/1;塞进 `bool` 时编译器**必须生成"!=0"的规范化代码**。
- 他平时的做法:**自己定义**

```c
typedef int32 bool32;      /* 只要"是不是 0",不要那个 0/1 规范化 */
```

  这样他就**不写 `!= 0`**,直接 `bool32 WasDown = (LParam & (1 << 30));`。
- 但**这一次他保留了 `!= 0`** —— 因为他后面确实要拿它和 `IsDown` **比较**(见 Day 006 第八节)。
- 他的取舍原话(大意):"我们其实浪费了一点时间(做那个 0/1 规范化)……但这次我们需要比较,所以得这么做。"

> 📌 **对我们的含义**:我们现在写 `bool` + `!= 0` 是**对的、也是清楚的**;
> `bool32` 属于"以后真做性能优化时"的选项 —— 正好呼应他自己反复讲的"别过早优化"。

---

## 五、XInput 的性能陷阱(以后必须回来修)

> `00:13:54` 起:"there is a bug in XInput… a really bad performance bug. And as far as I know, I'm the first person who actually complained about it"

- **现象**:对**没有插手柄**的索引调 `XInputGetState`,某些版本会 **stall 数毫秒**(他当年在 Molly Rocket 论坛上第一个抱怨这件事)。
- 我们现在每帧循环 4 个索引(我们的代码就是这么写的)—— **那些没插手柄的索引每帧都在付这个代价**。
- **文档的建议**(隔一段时间轮询一次)—— 他明确反对:**会让帧率变得一阵一阵的**。
- 他建议的正确做法:**用 HID 通知**(设备插拔事件),只在"确实连着"的手柄上轮询。
- 但**现在先不管**:`00:15:20` "we are not trying to be on the fast path right now" —— 这也正是他反复强调的:**别在东西还没跑通的时候去拧性能的旋钮**。

> 📌 **对我们的含义**:我们的 `for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ...)` 是**已知会 stall 的写法**,
> 现在**故意留着**;等做性能那一集再换成"HID 通知 + 只轮询连着的手柄"。

---

## 六、DirectSound:整体设计(他画的那张"糟糕的图")

### 为什么是 DirectSound

> `00:17:27` "We're going to be using DirectSound so that we'll have reasonable compatibility with older systems"

- 他想顺带保留**能编到 Windows XP** 的能力(顺带练"向后兼容"这回事)。
- 新 API 呢?`00:18:06` "There are newer audio APIs. I don't think any of them are particularly good anyway" —— 可能延迟好一点,但他不做新 API 的编程,不评价。

### ★ 环形缓冲 + "写在播放头前面"

| 事实 | 说明 |
|---|---|
| 缓冲是**环形**的 | DirectSound 播到末尾自动回到开头,无限循环 |
| 大小 | 他选 **≈2 秒**(后面 Q&A 解释为什么) |
| 采样率 | **48 kHz** = 每秒 48,000 样本(他还提到 44.1 kHz 是另一种常见值) |
| 每帧要写多少 | 48,000 ÷ 60 = **约 800 样本/帧**(30fps 则 1,600) |
| **关键** | **必须写在"播放位置前面一点"**,不能写在播放头当前位置 —— 因为**写入本身也有延迟**,硬写会把还没播的数据覆盖掉 |

### 关于"画面和声音对齐"的一段(值得单独记)

- 第 N 帧的画面会被用户看 1/60 秒;这段时间里我们在算**下一帧**的画面**和它对应的那段声音**;
- 理想状态:**把这段声音写进"和那次画面翻转(page flip)对齐的位置"**;
- 但**做不到**:以前 page flip 在垂直同步(vblank)上,现在**时机不可预测**(N 家同步、深流水线等等);
- 所以他的态度:`00:23:02` "it's just something to be aware of" —— 先记着,别追完美同步。

### 今天的实际目标

> `00:23:08` "just open a handle to DirectSound and get this buffer up… and have DirectSound just kind of do this infinite loop on it"

**注意他说这句话时还不知道今天能不能做完 —— 结果确实没做完(格式那一步炸了,下一集修)。**

---

## 七、加载三件套(和 Day 006 的 XInput 完全同构)

> `00:27:03` 起:"we're doing that exact same thing" —— 他明确说这就是**照抄 XInput 那套**。

| 步骤 | 做法 |
|---|---|
| 1 | `LoadLibraryA("dsound.dll")` —— 顺手强调:声卡/DSound 不存在**也不该让游戏打不开**,顶多没声音 |
| 2 | `GetProcAddress` 拿 `DirectSoundCreate` |
| 3 | 调用它,拿 DirectSound 对象 |

**和 XInput 的唯一区别**(他说得很清楚,`00:30:20` 附近):

- **不需要 stub、不需要全局函数指针** —— 因为这个函数**只会被调用一次**;
- 所以他只用了一个**局部**的 typedef + 局部函数指针,并且刻意**不做成全局**("免得别人误调用")。

### `DirectSoundCreate` 的真实签名(✅ `um\dsound.h` 原文)

```c
extern _Check_return_ HRESULT WINAPI DirectSoundCreate(
    _In_opt_ LPCGUID pcGuidDevice,
    _Outptr_ LPDIRECTSOUND *ppDS,
    _Pre_null_ LPUNKNOWN pUnkOuter);
```

| 参数 | 含义 |
|---|---|
| `pcGuidDevice` | 要打开**哪块**声卡;传 **0 = 默认设备**(他 ASR 里被认成 "grid device") |
| `ppDS` | **输出参数**:DirectSound 对象指针**通过它回给你**(不是返回值!) |
| `pUnkOuter` | COM 聚合用的东西。他的原话:`00:29:37` **"I have absolutely no idea… I have just always left it zero"** |

- 返回值是 `HRESULT`,要**检查**(DirectX 的规矩:用 `SUCCEEDED(hr)`)。
  ✅ 头文件实测(`shared\winerror.h`):`#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)` —— **就是"大于等于 0"**。
- ✅ 顺带解答他直播时的困惑:`dsound.h` 里是
  ```c
  typedef struct IDirectSound *LPDIRECTSOUND;
  ```
  也就是 **`DirectSoundCreate` 给你的是基础的 `IDirectSound`**(想用 8 的特性才需要 `DirectSoundCreate8`)。
  他当时纠结"这个类型到底是 8 还是不是",结论就是**用 `LPDIRECTSOUND` 够了**,因为我们只调最通用的功能。

---

## 八、★ 两个"缓冲":主缓冲**其实不是缓冲**

他在 Q&A 里被反复问,专门强调了三遍:**"There are not two buffers. This buffer is not really a buffer."**(`01:24:21` 附近)

| | 主缓冲(primary) | 次缓冲(secondary) |
|---|---|---|
| 本质 | **只是拿到"声卡"的句柄** | **这才是我们真正写入的缓冲** |
| 用途 | 调 `SetFormat` 告诉 Windows"按这个格式播" | `Lock` 写数据、`Play` 播放 |
| 创建标志 | `DSBCAPS_PRIMARYBUFFER`(✅ `0x00000001`) | 需要填 `lpwfxFormat` + 大小 |
| 大小 / 格式 | `dwBufferBytes = 0`、`lpwfxFormat = NULL`(**文档要求必须是 NULL**) | 我们要 48kHz / 16bit / 立体声 |
| 遗留性质 | **纯历史遗留**:老 DirectSound 时代你直接往声卡写 | 现在真正干活的 |

**为什么非要创建主缓冲**:只有在主缓冲上才能 `SetFormat`,把声卡置成**我们用的格式** ——
否则 44.1kHz 的声卡播 48kHz 数据会**来回重采样**(又慢又难听)。
他补了一句:现代机器上 kernel mixer 反正会兜底,所以这套主要对老机器有意义。

---

## 九、`SetCooperativeLevel`(协作级别)—— 以及"为什么必须等窗口建好"

- 调用形式:`IDirectSound::SetCooperativeLevel(HWND, DWORD level)`;
- ✅ 常量(`um\dsound.h`):`DSSCL_NORMAL = 1`、**`DSSCL_PRIORITY = 2`**、`DSSCL_EXCLUSIVE = 3`、`DSSCL_WRITEPRIMARY = 4`;
- 他选 **`DSSCL_PRIORITY`**:因为**只有 PRIORITY 及以上**才允许改主缓冲的格式;
- ⚠️ **它需要窗口句柄** ⇒ 这就是"**DirectSound 不能像 XInput 那样在 `WinMain` 开头初始化**"的原因:
  `00:24:03` "you can't really get your DirectSound going… until you have a window"。
  所以初始化必须放在**窗口创建成功之后**;
- 返回值他**没检查**(失败也做不了什么),只在失败分支里放个诊断。

---

## 十、`DSBUFFERDESC` 和那个"清零小技巧"

✅ 头文件原文(`um\dsound.h`):

```c
typedef struct _DSBUFFERDESC
{
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwBufferBytes;
    DWORD           dwReserved;
    LPWAVEFORMATEX  lpwfxFormat;
#if DIRECTSOUND_VERSION >= 0x0700
    GUID            guid3DAlgorithm;
#endif
} DSBUFFERDESC, *LPDSBUFFERDESC;
```

- 惯例:**传这种结构体前先清零**(他也吐槽"文档里没强调,但他们的例子全都这么做",猜是为了**以后加字段不破坏老代码**);
- 他的小技巧:因为 `dwSize` **正好是第一个成员**,他写成

```c
DSBUFFERDESC BufferDescription = {sizeof(BufferDescription)};   /* 第一个成员=大小,其余全 0 */
```

  然后又说这招"有点怪",**还是显式写更稳妥**(万一微软哪天挪了成员顺序);
- 主缓冲只需要:`dwFlags = DSBCAPS_PRIMARYBUFFER`、`dwBufferBytes = 0`、`lpwfxFormat = NULL`;
- 他看文档时对着一堆 flags(3D / frequency / pan / volume / global focus / sticky focus / loc hardware…)一路"这个我们不要" ——
  结论:**主缓冲几乎什么都不用填**。

---

## 十一、★ `WAVEFORMATEX` 的字段顺序坑(本集真正的 bug)

✅ 头文件原文的字段顺序(`um\mmeapi.h`):

```c
typedef struct tWAVEFORMATEX
{
    WORD  wFormatTag;       /* 格式类型 */
    WORD  nChannels;        /* 声道数 */
    DWORD nSamplesPerSec;   /* 采样率 */
    DWORD nAvgBytesPerSec;  /* 每秒字节数 */
    WORD  nBlockAlign;      /* 一个"块"的字节数 */
    WORD  wBitsPerSample;   /* 每个采样多少位(单声道) */
    WORD  cbSize;           /* 附加信息大小 */
} WAVEFORMATEX;
```

**坑在哪**:文档让你算

```
nAvgBytesPerSec = nSamplesPerSec × nBlockAlign
```

**但 `nBlockAlign` 在结构体里排在 `nAvgBytesPerSec` 后面** —— 你如果**按结构体顺序**往下填,
就会在 `nBlockAlign` **还是 0** 的时候去算 `nAvgBytesPerSec` ⇒ 填出 `nAvgBytesPerSec = 0`。

他填的值:

| 字段 | 值 | 说明 |
|---|---|---|
| `wFormatTag` | `WAVE_FORMAT_PCM`(✅ `mmeapi.h` 里 = **1**) | 最普通的 PCM |
| `nChannels` | 2 | 立体声(他后面 Q&A 解释:2D 游戏没必要更多) |
| `nSamplesPerSec` | 48,000 | 他把它做成**参数**,方便以后改 |
| `nBlockAlign` | **4** | = 声道数 2 × 16 位 ÷ 8 —— **必须先算这个** |
| `nAvgBytesPerSec` | 48,000 × 4 | **要在 `nBlockAlign` 之后算** |
| `wBitsPerSample` | 16 | "和 CD 一样";8 位也能用,但没必要 |
| `cbSize` | 0 | 没有扩展信息(他顺手把这个字段从结构体里"去掉"不写) |

**结果**:`SetFormat` 和"创建次缓冲"**两个调用都失败**,错误码 **`E_INVALIDARG`**
(✅ `winerror.h`:`#define E_INVALIDARG _HRESULT_TYPEDEF_(0x80070057L)`)—— 只告诉你"有个参数无效",**不告诉你是哪个**。

> 他的吐槽(`00:51:00` 起):"这才是 API 最不该有的东西:**一大堆失败点**"
> 以及 `01:05:16`:"**你在做 API 的时候别干这种事**……让人按结构体顺序填,别让某个成员依赖后面才填的成员。"

**直播弹幕抓到了它**(`01:04:06`),他改的顺序是:**先填 `nBlockAlign`,再算 `nAvgBytesPerSec`** ⇒ 一次通过:

> `01:04:42` "Holy cow, dude. **That was it.**"
> `01:04:46` "So just me misplacing the structure thing… **that was the only bug we had.**"

> 📌 **注意一个反直觉的点**(他自己也讲了):**把结构体清零反而掩盖了这个 bug** ——
> 如果不清零,编译器可能会提醒"你用了没初始化的成员"(`C4700`);清零之后"用了 0"看起来完全合法。

---

## 十二、这一集的结果清单

| 步骤 | 结果 |
|---|---|
| `LoadLibraryA("dsound.dll")` | ✅ |
| `DirectSoundCreate` 取地址 + 调用 | ✅ |
| `SetCooperativeLevel(hwnd, DSSCL_PRIORITY)` | ✅ |
| 创建**主缓冲** | ✅ |
| `SetFormat(主缓冲, &WaveFormatEx)` | ❌ `E_INVALIDARG` → 改顺序后 ✅ |
| 创建**次缓冲** | ❌ 同上 → 改顺序后 ✅ |
| 开始播放(`Play`) | ⏸️ **今天没做**(他说要等真正写数据时再做) |
| `Lock` 写入声音 | ⏸️ **下一集** |

> 文档最后一句(`01:32:49`):"see you tomorrow morning **when we will actually start playing some sounds out of our sound buffer**."

---

## 十三、Q&A 要点(挑跟代码有关的)

| 问题 | 他怎么说 |
|---|---|
| **为什么要两个缓冲?** | 他说"老实讲我解释不了,只能说 Windows 就是这么怪" —— 真答案见第八节(主缓冲只是拿句柄改格式)。**反复强调"这不是双缓冲"** |
| **掉帧会不会爆音?** | **会**,而"2 秒大缓冲"就是对策:宁可**干脆停更 2 秒**(播缓冲里的旧数据,基本听不出来),也不要**窄缓冲反复循环**那 800/1600 个样本——那种"哒哒"声耳朵立刻能听出来 |
| `nBlockAlign` 和 `nAvgBytesPerSec` 冗余吗? | **冗余**(John 指出)。他同意:"既然冗余,为什么不让我们传 0 让你自己算?" |
| 会用到多声道吗? | 2D 游戏**不需要**;沉浸式 3D(5.1/7.1)才有意义 |
| 该用新 API 吗? | "**XAudio** 大概是现在大家用的",但他不做新 API 编程,不评价 |
| Linux 用什么? | 大概 **ALSA**(最普适);他会去问有 Linux 出货经验的人(比如 Valve 的朋友) |
| 骨骼动画 vs sprite sheet? | **两者不互斥**(sprite 也能绑骨骼);他说"大概会支持某种弯曲",取决于渲染器 |
| 音频预载还是流式? | **流式**(stream from disk) |
| 会有多线程/并发吗? | **会** |
| **`VirtualAlloc` 的更正**(与我们直接相关) | 应该**同时传 `MEM_RESERVE | MEM_COMMIT`**;只传 COMMIT 也能工作,但规范上不对(有 Raymond Chen 的文章为证) —— 他承认自己以前也一直只传 COMMIT |
| "为什么不干脆用引擎?" | 见附录 B 第 3 条(马车 vs 火箭) |

---

## 十四、和我们代码的关系 / 下一步(重要)

> ✅ 先说清楚:**音频属于"引擎层",和游戏类型无关** —— 我们做**类银河城**,照样要音乐/音效/环境音,
> 这一集的 DirectSound 那套照学不误;以后自己设计的只是"什么时候放什么声音"这种**玩法层**的东西。

| # | 要做的事 | 依据 | 优先级 |
|---|---|---|---|
| 1 | **两个 stub 改成 `return(ERROR_DEVICE_NOT_CONNECTED);`** | 本集 `00:01:17` 明确更正 | 高(现在的 `return 0` 是**语义错误**) |
| 2 | **`Win32LoadXinput()` 改成"先 1.4 再 1.3"** | 本集 `00:02:49` | 中(我们机器上不会失败,但照他的写) |
| 3 | ★ **Alt+F4 自己处理**:`AltKeyWasDown = ((LParam & (1 << 29)) != 0);` + `if (VKCode == VK_F4 && AltKeyWasDown) Running = false;` | 本集 `00:03:44` | **高** —— 我们上一轮合并了四个 case 标签之后,Alt+F4 已经失效(我标过"未验证的副作用",现在被证实) |
| 4 | `Win32LoadXinput()` 补"取不到导出函数就退回 stub"的兜底 | Day 006 就欠着 | 中 |
| 5 | `VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)` | 本集 Q&A | 低(行为不变,只是更规范) |
| 6 | **DirectSound 全套**(LoadLibrary → Create → 协作级别 → 主缓冲 → `WAVEFORMATEX` → 次缓冲) | 本集主体 | **这是下一集的活** |
| 7 | `bool32` 之类 | 本集 `00:09:17` | 低(等性能阶段) |

**还有两条"记着但现在不要做"的**:

- ⚠️ **XInput 的 stall**:我们那个"每帧轮询 4 个索引"的循环**已知会卡**,等性能那一集再改成 HID 通知;
- ⚠️ **音画同步**:第 N 帧画面 + 它那段声音要一起出现;现在**不追求完美同步**(page flip 时机不可预测)。

---

## 附录 A:本集新出现的 API / 常量速查(值都在本机 SDK 头文件里核实过)

| 名字 | 作用 | 出处 / 实测值 |
|---|---|---|
| `LoadLibraryA("dsound.dll")` | 运行时加载 DirectSound | ✅ `C:\Windows\System32\dsound.dll` 存在 |
| `DirectSoundCreate` | 拿 DirectSound 对象(默认设备) | `um\dsound.h`:`HRESULT WINAPI DirectSoundCreate(LPCGUID, LPDIRECTSOUND *, LPUNKNOWN)` |
| `DirectSoundCreate8` | 拿 **8** 版的接口(他**故意不用**,不想要求 8) | 同上 |
| `LPDIRECTSOUND` | = `struct IDirectSound *` | `um\dsound.h` ✅ |
| `HRESULT` / `SUCCEEDED(hr)` | 返回码 / "成功"判定 | `winerror.h`:`((HRESULT)(hr)) >= 0` ✅ |
| `E_INVALIDARG` | "参数无效"(他这次撞的错) | `0x80070057` ✅ |
| `IDirectSound::SetCooperativeLevel` | 设协作级别(要窗口句柄) | —— |
| `DSSCL_NORMAL` / `DSSCL_PRIORITY` | 协作级别 | `1` / **`2`** ✅ |
| `DSBUFFERDESC` | 创建缓冲的描述结构 | 字段顺序见第十节 ✅ |
| `DSBCAPS_PRIMARYBUFFER` | "我要主缓冲"标志 | `0x00000001` ✅ |
| `IDirectSound::CreateSoundBuffer` | 创建缓冲(主/次都是它) | —— |
| `IDirectSoundBuffer::SetFormat` | 设主缓冲格式 | —— |
| `WAVEFORMATEX` | 音频格式结构 | 字段顺序见第十一节 ✅ |
| `WAVE_FORMAT_PCM` | 普通 PCM 格式 | `1` ✅ |
| `IDirectSoundBuffer::Lock` / `Play` | 写数据 / 播放 | `DSBPLAY_LOOPING = 0x1`、`DSBLOCK_ENTIREBUFFER = 0x2` ✅(**下一集用**) |
| `DuplicateSoundBuffer` | 复制缓冲 —— **共享同一块内存,不能用** | 他在 `00:52:43` 查完否掉了 |
| `VK_F4` | F4 的虚拟键码 | `0x73` ✅ |
| `LPARAM` bit29 | context code:**Alt 是否按下** | 本集 `00:04:48` 查文档 |
| `MEM_RESERVE | MEM_COMMIT` | `VirtualAlloc` 更规范的用法 | 本集 Q&A |

---

## 附录 B:本集值得抄下来的几句话

1. **关于错误码约定**(`00:02:12`):
   > "that was just me being absent in mind and forgetting that **zero is my kind of error code return, not Windows**."
2. **关于自造 API 的规矩**(`01:05:16` 附近):
   > "**You want to initialize the variables in the order they appear in the structure.** Don't stick variables down somewhere and then tell the user to use them in a line that happens before that member. **You're just asking for bugs.**"
3. **关于"为什么不自己写平台代码"**(`00:56:00` 起,这是本集的精神内核):
   > "A lot of people always say it's so complicated and why would you write it yourself? … **how long does it take to just write that function?** … maybe a day, **but then you've got it forever.**"
4. **马车 vs 火箭**(`01:29:32` 起,回答"为什么不直接用引擎"):
   > "**We don't have any wheels right now.** … Don't pick up a game engine and think you are using a rocket. **You are using a horse-drawn carriage.** And what we need to get to over time is the rocket."
   > "We are not even 10 years from that day. We are like **50 years** from that day if we're lucky."

---

## 附录 C:日程与其它(他亲口说的)

| 时间 | 内容 |
|---|---|
| 次日(周三) | **真正把声音从缓冲里播出来**(`Play` + `Lock` 写数据) |
| 周三 / 周四 / 周五 | 感恩节假期 ⇒ **改成上午 11:00 直播** |
| 很快 | `handmadehero.org` 的"邮件提问"按钮要变成**论坛**(还在折腾 Joomla) |

---

## 附录 D:本文档的事实核对来源

| 声称 | 怎么核实的 |
|---|---|
| `DirectSoundCreate` / `LPDIRECTSOUND` / `DSBUFFERDESC` / `DSBCAPS_PRIMARYBUFFER` / `DSSCL_*` | `C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\um\dsound.h`(19041 / 22621 / 26100 三个 SDK 都有这个头文件) |
| `WAVEFORMATEX` 字段顺序 / `WAVE_FORMAT_PCM = 1` | 同目录 `um\mmeapi.h` |
| `SUCCEEDED` / `E_INVALIDARG` / `ERROR_SUCCESS = 0` / `ERROR_DEVICE_NOT_CONNECTED = 1167` | `shared\winerror.h` |
| `VK_F4 = 0x73` | `um\winuser.h` |
| `dsound.dll` 是否存在 | ✅ 本机 `Test-Path C:\Windows\System32\dsound.dll` = True |
| `XInput1_4 / 1_3 / 9_1_0` 三个 DLL 都在 | ✅ 本机 `C:\Windows\System32\` 实测 |
| **运算符优先级那个 bug** | ✅ 本机编译运行 `build\precedence_test.cpp`:他写的得到 `1`(错),正确写法得到 `0` |
| **`C4554` 警告** | ✅ 同一次编译输出的原话:`warning C4554: "&": 检查运算符优先级是否存在的可能的错误` |
| 时间戳引用 | 来自 `explain\!subtitle\[Mira] Handmade Hero Day 007 - Initializing DirectSound.txt`(⚠️ ASR 文本,数字/名字可能有误) |
