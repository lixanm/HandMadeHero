---
集数: "008"
英文标题: Writing a Square Wave to DirectSound
别名:
  - Day 8
  - 方波
  - 写方波到 DirectSound
标签:
  - handmade-hero
  - day/008
  - 主题/音频
  - 主题/COM
  - 主题/数学
  - 主题/Q&A
相关代码: HandMade/code/win32_handmade.cpp
字幕: "explain/!subtitle/[Mira] Handmade Hero Day 008 - Writing a Square Wave to DirectSound.txt"
状态: 已完成
---

# Day 008 - Writing a Square Wave to DirectSound

> **本篇导航**:一、这一集干了什么(总览) · 二、★ 开场:COM 与 vtable(为什么 `DirectSound->Method()` 能用) · 三、回顾昨天的初始化(以及 48kHz 是怎么定的) · 四、★ 把次缓冲指针升为全局(附他关于全局变量的规矩) · 五、★ 方波的数学(这一集的核心新知识) · 六、★ `Lock` 为什么可能返回**两块**区域 · 七、★ `GetCurrentPosition`:play cursor 与 write cursor · 八、★ 写缓冲的完整算法 · 九、`Play(0, 0, DSBPLAY_LOOPING)`:让它响 · 十、★ 现场:响了 → 太吵 → 咔哒声 → 忘了 `Unlock` · 十一、Q&A 前他当场补的两处(和"启动一秒后才出声"有关) · 十二、Q&A 要点(从 `01:10:32` 到 `01:31:15`) · 十三、和我们代码的关系 / 下一步 · 十四、★ 音频系统全景(我们这份代码是怎么实现的) · 附录 A–F

> **这一集讲了什么**:先用一大段**把 COM/vtable 讲透**(`DirectSound->Method()` 到底发生了什么),
> 然后**把方波写进次缓冲**:算频率与周期、`GetCurrentPosition` 拿播放游标、`Lock`/`Unlock` 两段写入、`Play` 循环播放。
> **结果**:真的响了 ✅ —— 但先"**太吵**",接着有"**咔哒声**",**弹幕提醒他忘了 `Unlock`**,补上后正常。
> 结尾他承认**这块环形缓冲代码八成还有 bug**,明天专门调试(并改用正弦波,因为更容易听出问题)。
>
> [!warning]
> 证据等级(沿用 `NOTES.md` §2.2):引用都带**时间戳**;字幕是 **ASR**,API 名/数字常有错;
> 凡 **API 签名、常量值**都在本机 SDK 头文件里核实过(出处见附录 D)。**🟡** = 推断。

---

## 一、这一集干了什么(总览)

| 段落 | 内容 | 时间 |
|---|---|---|
| 1 | ★ **开场大段**:**COM 与 vtable** —— `DirectSound->SetCooperativeLevel()` 背后编译器做了什么 | `00:01:38` 起 |
| 2 | 回顾昨天的初始化;顺带交代 **48kHz 的来历** | `00:13:24` 起 |
| 3 | 把**次缓冲指针升为全局** + 他关于"什么时候才配当全局变量"的三条判据 | `00:15:02` 起 |
| 4 | ★ **方波的数学**:Hz / 周期 / 半周期 / 取 256(接近中音 C) | `00:15:59` 起 |
| 5 | ★ **`Lock` 为什么返回两块区域**(环形缓冲复习) | `00:18:19` 起 |
| 6 | ★ **`GetCurrentPosition`:play cursor 与 write cursor**(异步硬件) | `00:36:05` 起 |
| 7 | ★ **写缓冲的完整算法**(两种情形 + `runningSampleIndex` + mod) | `00:41:40` 起 |
| 8 | `Play(0, 0, DSBPLAY_LOOPING)` —— **真的响了** | `00:56:13` / `00:57:40` |
| 9 | ⚠️ "太吵" → 音量参数化;"咔哒声" → **弹幕提醒忘了 `Unlock`** | `00:57:55` / `00:59:27` |
| 10 | Q&A 前**补两处**:先填满再播、以及 `playCursor == ByteToLock` 的边界情形 | `01:18:14` 起 |
| 11 | Q&A(近 20 分钟):XAudio2/XP、输入、**延迟**(含著名的 no poop sauce) | `01:10:32` ~ `01:31:15` |

---

## 二、★ 开场:COM 与 vtable(为什么 `DirectSound->Method()` 能用)

> 这一段的起因:他回到昨天写的那个 DirectSound 初始化函数(我们的代码里叫 `Win32InitSound`;这一集他顺手给它加了个缓冲区大小的参数,我们那份正在改的代码也照做了),那个 `DirectSound->SetCooperativeLevel(...)` 里的**小箭头**让他觉得"该讲清楚这是怎么回事"。原话大意:`00:01:38` "I think it's worth explaining how this works"。

### 2.1 先复习 struct

`struct` 只是**告诉编译器"这块内存怎么摆"**:成员按声明顺序、按各自大小排布(中间可能有 padding)。
`00:02:43` "it tells the compiler to lay out a particular piece of memory"。

### 2.2 成员函数:`this` 是编译器偷偷塞进去的参数

C++ 允许在 struct 里声明函数(用 `Foo::bar` 这种 **scoping operator** 定义)。看起来只是"语法糖",但编译器**真的改写了函数签名**:

- 会插入一个**合成的参数 `this`**(指向调用它的那个对象);
- 函数体里直接写 `x`,编译器会先去 `this` 里找,找到就自动改成 `this->x`;
- `00:05:45` 原话:"that is the only real difference… it allows you to have this free sort of lookup"。

### 2.3 `virtual`:多出一张**全局表**,对象里多藏一个指针

一旦声明成 `virtual`(DirectSound 全是 virtual),就不只是语法层面的事了:

| | 普通函数 | **virtual 函数** |
|---|---|---|
| 调用点生成的代码 | `call <常量地址>`(链接器写死) | 先读对象的 **vtable 指针** → 再读表里那一格 → `call` 寄存器 |
| 编译器额外准备 | 无 | 为每个"有虚函数的类型"生成一张 **vtable(全局函数指针表)** |
| 代价 | 一次直接跳转 | **两次间接寻址**(double indirection) |

`00:07:01` 原话:"a vtable is basically something that contains a bunch of function pointers"。
`00:07:45` 他还专门点出:**"这和我们为 XInput 手写的函数指针是同一件事"** ——
区别只是我们用的是**一个全局指针**,而编译器用的是**一张全局表**(每个类型一张)。

下面把这件事**一层一层拆开**讲透。这一节是本集里"技术含量最高"的一段,值得慢一点读。

---

#### 2.3.1 先问:为什么普通函数就不需要这张表?

一条 `call` 指令后面要跟"跳到哪",而这个地址是**链接器写死进 exe 的**。
对我们自己写的函数来说这完全没问题 —— 因为链接器**现在就看得见它**。

我们可以在**自己的 exe** 里同时看到四种"跳到哪"的写法(下面这四行都是真的从
`HandMade\build\win32_handmade.exe` 里 `dumpbin /disasm` 出来的):

| 调用形式 | 汇编长什么样 | "跳到哪"是谁决定的 |
| --- | --- | --- |
| **同模块内的普通函数**<br>(如 `WinMain` 里调 `Win32InitSound`) | `call 0000000140001630` | **链接器写死**(常量地址) |
| **别的 DLL 导出的普通函数**<br>(如 `LoadLibraryA`) | `call qword ptr [__imp_LoadLibraryA]` | 链接器写死**表里的哪一格**;运行期只填基址(ASLR 才需要) |
| **我们手写的函数指针**<br>(XInput / `DirectSoundCreate`) | `call qword ptr [rsp+48h]` | **运行期**:`GetProcAddress` 的返回值,存在一个变量里 |
| **虚函数**<br>(`DirectSound->SetCooperativeLevel`) | `mov rax,[对象]`<br>`call qword ptr [rax+30h]` | **槽位编译期定**(`0x30`)、**表运行期定**(对象里那个指针) |

前三种都还能"提前把目标钉住":

- 同一个模块 → 直接写死地址;
- 别的 DLL 的**导出函数** → 给它在**导入地址表(IAT)** 里留一格,链接器写死"第几格",
  运行期系统把 DLL 的真实基址填进去(这就是为什么 `user32.lib` 那种"导入库"里其实没有代码,
  只有"让链接器生成 IAT 格子"的信息 —— 见 Day 007 附录 E);
- 我们手写的函数指针 → 连"第几格"都可以是我们自己定的变量。

**但虚函数不行。** 原因是:

1. **真正的实现在 `dsound.dll` 里,而且可能不止一种**(DirectSound8 就是另一套);
   调用点写这段代码的时候,**根本不知道会拿到哪个对象**;
2. 就算知道了,**"该跳到哪"这件事必须跟着对象走** —— 同一个函数名,对不同实现要跳不同地方;
3. 于是最自然的解法是:**把"跳到哪"从指令里挪出来,变成数据**。
   对象里存一个指针,指向"这个对象该用的那张函数指针表"。

这就是 `virtual` 干的事 —— 它**不是语法糖**,它真的改了对象的内存布局和调用点的指令。

> [!info]
> 一句话总结:`virtual` 把"调用哪个函数"这个问题,从**编译期的指令**变成了**运行期的数据**。

---

#### 2.3.2 布局图:对象里藏的是什么,那张表长什么样

`DirectSoundCreate` 交还给我们的那个 `LPDIRECTSOUND`,**不是一个句柄,而是这样一个对象**:

```
   LPDIRECTSOUND DirectSound                 dsound.dll 映像里的那张表
   (这个"对象"只有 8 字节)                    (随 DLL 加载被映射进内存;
            +-----------------+               由 DLL 提供,不在我们的 exe 里)
 DirectSound| lpVtbl          |              +---------------------------------+
   -------->| (偏移 0)         |------------>| [0]  +00h   QueryInterface      |
            +-----------------+              | [1]  +08h   AddRef              |
                                             | [2]  +10h   Release             |
                                             | [3]  +18h   CreateSoundBuffer   | ← 我们建主/次缓冲用的
                                             | [4]  +20h   GetCaps             |
                                             | [5]  +28h   DuplicateSoundBuffer|
                                             | [6]  +30h   SetCooperativeLevel | ← 就是这一格(0x30!)
                                             | [7]  +38h   Compact             |
                                             | [8]  +40h   GetSpeakerConfig    |
                                             | [9]  +48h   SetSpeakerConfig    |
                                             |[10]  +50h   Initialize          |
                                             +---------------------------------+
```

四个要点:

1. **一张表 ≠ 一个对象。** 表是"**每个有虚函数的类型**一张",所有 `IDirectSound` 对象**共用同一张**;
   对象里存的只是"**我指向哪张表**"。这就是为什么它可以"同一种调用代码服务不同实现"。
2. **表在 DLL 里,不在我们 exe 里。** `00:09:16` 他的原话:"the vtable got mapped into memory
   when we loaded the dll" —— 我们拿到的对象里那个指针,指进 `dsound.dll` 自己的数据区。
3. **vtable 指针在对象的偏移 0。** MSVC 在"单继承、无虚基类"的情况下就是这么摆的(我们从
   汇编里直接看到了这一点,见 2.3.4)。这带来一个有趣结论:**COM 接口的"对象"本体就只有一个指针那么
   大(8 字节)** —— 因为它只有行为、没有数据成员。
4. **`IUnknown` 的三格永远在最前面**(`QueryInterface` / `AddRef` / `Release`)。
   这不是巧合,而是 COM 的地基:任何接口的前三格都一样,所以任何接口都能被"问出"别的接口。
   📍 这也解释了我们代码里的一个现象:`DirectSound` 我们**从来没见过它怎么被释放** ——
   因为"引用计数 + `Release`"就是这三格里的事,而我们连 `Release` 都没调(不优雅,但在单进程小工具里
   后果只是"进程退出时一起没了")。

---

#### 2.3.3 ★ "第几格"是**编译期**就定死的 —— 头文件的声明顺序就是表的顺序

这是整件事里最容易被忽略、也最值得记住的一点。

我们本机的 `dsound.h`(`C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\um\dsound.h`,
第 594–610 行)是这么声明的:

```c
DECLARE_INTERFACE_(IDirectSound, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ _In_ REFIID, _Outptr_ LPVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectSound methods
    STDMETHOD(CreateSoundBuffer)    (THIS_ _In_ LPCDSBUFFERDESC, ...) PURE;
    STDMETHOD(GetCaps)              (THIS_ _Out_ LPDSCAPS) PURE;
    STDMETHOD(DuplicateSoundBuffer) (THIS_ _In_ LPDIRECTSOUNDBUFFER, ...) PURE;
    STDMETHOD(SetCooperativeLevel)  (THIS_ HWND hwnd, DWORD dwLevel) PURE;
    STDMETHOD(Compact)              (THIS) PURE;
    STDMETHOD(GetSpeakerConfig)     (THIS_ _Out_ LPDWORD) PURE;
    STDMETHOD(SetSpeakerConfig)     (THIS_ DWORD) PURE;
    STDMETHOD(Initialize)           (THIS_ _In_opt_ LPCGUID) PURE;
};
```

`SetCooperativeLevel` 是**第 7 个**(从 0 数起就是第 6 格),所以它的槽位偏移 = `6 × 8 = 48 = 0x30`。
编译器看到 `DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY);` 时,只做两件事:

1. 从对象**偏移 0** 取出**表指针**(这是运行期才知道的值);
2. 把**偏移 `0x30`** 当成常量写进指令。

⇒ 这就是 `call qword ptr [rax+30h]` 里那两部分的来历:
**`0x30` 是编译期数出来的,`rax` 是运行期读出来的。**

**★ 头文件里的顺序 = ABI 契约。** 更妙的是,`dsound.h` 自己就把这件事写在明面上:
它在 616–624 行还提供了一份 **C 语言版本**的宏(因为 C 里没有 `virtual`):

```c
/* C 版本:双重间接被明明白白写出来了 */
#define IDirectSound_SetCooperativeLevel(p,a,b)  (p)->lpVtbl->SetCooperativeLevel(p,a,b)
/*                                                    ↑ 第 1 跳  ↑ 第 2 跳        ↑ this 又传了一遍 */
```

而 C++ 分支(625–634 行)是:

```c
#define IDirectSound_SetCooperativeLevel(p,a,b)  (p)->SetCooperativeLevel(a,b)
/*                                                    ↑ 编译器替你做了上面那两步 */
```

看出来了吗?**你写的那一行 `DirectSound->SetCooperativeLevel(...)`,展开后就是
"两次间接 + 把 `DirectSound` 自己当第一个参数传回去"** —— 后者正是 2.2 节说的那个隐藏的 `this`。

由此可以推出两条很实用的结论:

- 表的顺序一旦发布就**不能改**:改了,所有已经编译好的程序全会跳到错函数上
  (这就是 COM 的规矩:接口只能**末尾追加新方法并换一个新 IID**,不能插队、不能重排);
- 你在 C 里想手写这套调用**完全可行**(就是 `p->lpVtbl->Method(p, ...)`)——
  这也是为什么"用 C 写 COM"一点问题都没有,只是啰嗦。

---

#### 2.3.4 ★ 实证:我们**自己 exe** 里的那几条指令

他是在调试器里看的(`00:11:46`)。我们不用靠记忆 —— `Win32InitSound` 里的这段汇编就在
`HandMade\build\win32_handmade.exe` 里,`dumpbin /disasm` 直接读得出来:

```
00000001400016AF: 48 8B 44 24 20     mov         rax,qword ptr [rsp+20h]
00000001400016B4: 48 8B 00           mov         rax,qword ptr [rax]
00000001400016B7: 41 B8 02 00 00 00  mov         r8d,2
00000001400016BD: 48 8B 94 24 D0 00  mov         rdx,qword ptr [rsp+0D0h]
00000001400016C5: 48 8B 4C 24 20     mov         rcx,qword ptr [rsp+20h]
00000001400016CA: FF 50 30           call        qword ptr [rax+30h]
```

逐行翻译:

| 行 | 在干什么 |
| --- | --- |
| `mov rax,[rsp+20h]` | `rax` = 局部变量 `DirectSound`(对象指针) |
| `mov rax,[rax]` | **第 1 跳**:读对象**偏移 0** → 取到 vtable 指针 |
| `mov r8d,2` | 第 3 个参数 = `DSSCL_PRIORITY`(= 2) |
| `mov rdx,[rsp+0D0h]` | 第 2 个参数 = `Window`(从 shadow space 里取) |
| `mov rcx,[rsp+20h]` | **第 1 个参数 = 对象自己** ← ★ 这就是 2.2 说的隐藏 `this` |
| `call qword ptr [rax+30h]` | **第 2 跳**:表里第 6 格 → 间接调用 |

**一张图对上了三件事**:`this` 是第一个参数(`rcx`)、vtable 指针在偏移 0、槽位 `0x30` 是编译期常量。

同一个函数里还能捡到另外几组对照,全都和头文件顺序严丝合缝:

| 汇编 | 是哪一次调用 | 对得上头文件里的第几格 |
| --- | --- | --- |
| `call qword ptr [__imp_LoadLibraryA]` | `LoadLibraryA("dsound.dll")` | IAT —— 不是 vtable |
| `call qword ptr [rsp+48h]` | `DirectSoundCreate(...)` | 我们手写的函数指针 —— 也不是 vtable |
| `call qword ptr [rax+18h]` | `DirectSound->CreateSoundBuffer(...)` ×2 | `0x18 / 8 = 3` → `IDirectSound` 第 3 格 ✅ |
| `call qword ptr [rax+70h]` | `PrimaryBuffer->SetFormat(...)` | `0x70 / 8 = 14` → `IDirectSoundBuffer` 第 14 格 ✅ |

> [!question]
> **证据出处说明**:上面这份汇编来自 `HandMade\build\win32_handmade.exe`(2026-09-24 21:14 编译,
> 即**已提交版**源码的产物)。因为用了 `-Od`(不优化),编译器把每一步都老老实实写出来了;
> **开优化之后可能不一样** —— 比如它可以把这个对象的 vtable 指针缓存进寄存器反复用、
> 甚至证明"这里只有一个可能类型"而直接把目标写死(devirtualization)。
> 也就是说:**上面那条路径是"最坏情况"的形态,不是唯一形态**。

---

#### 2.3.5 和他给 XInput 手写的那一套,是**同一件事**

`00:07:45` 他说了这句话("这和我们为 XInput 手写的函数指针是同一件事")。为什么?

因为我们当时干的事是:**把"函数在哪"从指令里挪出来,变成数据**。区别只在"几个函数"和"谁填这张表":

```c
/* ① 我们 Day 006 的 XInput:只有一个函数要取 → 一个全局变量就够了 */
typedef DWORD WIN32_XINPUT_GET_STATE(DWORD, XINPUT_STATE *);
global_variable WIN32_XINPUT_GET_STATE *XInputGetState_;
...
XInputGetState_ = (WIN32_XINPUT_GET_STATE *)GetProcAddress(Library, "XInputGetState");

/* ② 如果有一整排函数要取 → 把它们装进一个 struct,就是"手搓 vtable" */
struct xinput_api
{
    WIN32_XINPUT_GET_STATE   *GetState;
    WIN32_XINPUT_SET_STATE   *SetState;
    WIN32_XINPUT_GET_CAPABILITIES *GetCapabilities;
};
/* 调用点同样变成:先取表指针,再按固定偏移取那一格 */

/* ③ DirectSound 的做法又往前走了一步:这张表不由我们填,而是藏在对象里,
      由 DirectSoundCreate 连"表"带"对象"一起交给我们 —— 于是同一份调用代码
      可以被不同实现复用。这就是"接口"的全部含义。 */
```

差别整理成一张表:

| | 我们手写的 XInput 版 | 编译器生成的 vtable |
| --- | --- | --- |
| 表在哪 | 我们 exe 的全局数据区 | **提供实现的那个 DLL 里** |
| 谁填表 | 我们在 `Win32LoadXinput` 里填 | DLL 的作者(编译时生成) |
| 表怎么选 | 一个全局变量,选定就不变 | **每个对象自带一个指针**,可以指向不同的表 |
| 表知道"是哪个实例"吗 | 不知道(XInput 没有实例概念) | 知道 —— 每个函数的第一个参数都是 `this` |
| 调用点长什么样 | `call qword ptr [全局变量]` | `mov rax,[对象]` + `call qword ptr [rax+0x30]` |

**为什么 XInput 不需要对象?** 因为 XInput 没有"每个实例各自的状态" ——
手柄用 0/1/2/3 这种索引区分,函数本身是**无状态**的,所以一个全局指针就够。
而 DirectSound 的每个缓冲对象都有自己的状态(格式、位置、播放状态),
方法必须知道"你说的是哪一个",所以必须把"表指针"跟着对象走 —— 这也是"**第一次间接是为了找到对象自己的表,
第二次才是找到函数**"的原因。

---

#### 2.3.6 代价:两次间接寻址,换来"一份代码服务任何实现"

| | 普通函数 | 虚函数 |
| --- | --- | --- |
| 调用点指令数 | 1 条 `call` | 多一次 `mov`(取表指针)+ 1 条间接 `call` |
| 优化器 | 能看到目标 → **可以内联** | 通常看不到 → **不能内联** |
| CPU | 直接跳转,好预测 | 间接跳转,预测更吃力(目标数据相关) |
| 出错时你要多走一步 | —— | 得先弄清"这个对象是哪张表" |

但请注意**量级**:`SetCooperativeLevel` 这种函数**一辈子只调几次**,
多读一次内存、多一次间接跳转,代价**等于 0**。真正付出代价的是
"**人和优化器都看不出会跳到哪**" —— 这对**每帧要调几万次**的函数才要紧
(这也是为什么游戏代码里会尽量避免热路径上的虚调用,宁可换成显式函数指针)。

顺着他 `00:10:20` 的那句话理解就更清楚了:

> "everything gets translated to CPU… languages make things simpler for the programmer,
> they do not make things any simpler for the CPU."

**语言层给你的是"方便",CPU 最后拿到的还是"取址、取址、调用"。**
我们这一集做的事,恰恰是**把语言层的神秘感剥掉,看到底下那几条 `mov` / `call`**。

最后照例标注不确定的地方:`00:12:52` 他本人说
"I'm not the best person at Assembly Language… I'm just basically assuming that this is roughly what it has to do" 🟡 ——
**而我们的这份汇编是在自己机器上 `dumpbin` 出来的实测结果**,不依赖他的口述。

---

### 2.4 三者的关系:`DirectSoundCreate` / `DirectSound` / `SetCooperativeLevel`

> 为什么单独开一节:**这三个名字长得像,层级却完全不同** ——
> `DirectSoundCreate` 是**函数**,`DirectSound` 是**变量**(装的是对象指针),
> `SetCooperativeLevel` 是**对象上的方法**。下面用 7 张图把它们的**归属 / 先后 / 依赖**三件事摆平。
> 提到的头文件行号都在本机 `dsound.h`(`C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\um\dsound.h`)上核过。

#### 图 1:三层归属

```
        我们的 exe (win32_handmade.cpp)                dsound.dll(一个文件)
   +--------------------------------------+   +----------------------------------------+
   | (1) 取地址:普通函数,不属于任何对象      |   | 导出表(exports,可以按名字取)            |
   |   direct_sound_create *DirectSoundCreate|  |   DirectSoundCreate  <- 平铺的普通函数   |
   |        | = GetProcAddress 拿到的地址    |-->|                                        |
   |        v                              |   | 代码段:真正的实现                        |
   | (2) 出生:拿到对象                      |   |   SetCooperativeLevel / CreateSoundBuffer|
   |   LPDIRECTSOUND DirectSound = 0;      |   |   ...                                  |
   |        |  <- 一个 8 字节的变量           |   |                                        |
   |        v                              |   | 数据段:IDirectSound 的 vtable           |
   | (3) 用对象:方法,靠两次间接             |   |   [0] QueryInterface                   |
   |   DirectSound->SetCooperativeLevel(..)|-->|   [6] SetCooperativeLevel  <- 偏移 0x30 |
   +--------------------------------------+   +----------------------------------------+
```

- **`DirectSoundCreate`** 是 dsound.dll **导出的普通函数**(`dsound.h` L471:`extern _Check_return_ HRESULT WINAPI DirectSoundCreate(...)`),
  所以要用 `GetProcAddress` **按名字取地址** —— 这就是我们 Day 006/007 练的那套。
- **`DirectSound`** 是我们**自己声明的一个变量**(我们代码 L125),类型 `LPDIRECTSOUND`,
  而 `LPDIRECTSOUND` 只是 `typedef struct IDirectSound *LPDIRECTSOUND;`(`dsound.h` L166)
  ⇒ 它装的就是"指向 `IDirectSound` 对象的指针"。
- **`SetCooperativeLevel`** 是**那个对象上的一个虚方法**(`dsound.h` L605,`IDirectSound` 的第 6 格),
  通过对象里的 vtable 跳过去(偏移 `0x30`,见 2.3.4 的实测汇编)。

#### 图 2:鸡生蛋 —— 为什么 `DirectSoundCreate` 只能是"普通函数"

```
   想调 SetCooperativeLevel ---需要---> 一个 IDirectSound 对象
                                            |
                                            | 那这个对象从哪来?
                                            v
                            DirectSoundCreate(0, &DirectSound, 0)
                                            |
        * 它不能是方法 --- 因为"调方法"这件事本身就需要一个对象 *
                                            |
                                            v
                        所以它必须是平铺的、可以直接调的导出函数
```

**COM 的通则:所有"出生"函数都是普通函数;出生之后的一切都是方法。**
(更"正规"的路子是 `CoCreateInstance`,那是"工厂函数 + 注册表"的另一套;
DirectSound 用了更老的简化入口,对我们反而更省事,也不用懂 COM。)

#### 图 3:调用时序(每个参数谁写谁)

```
我们的代码                                          dsound.dll 内部
----------                                        --------------
GetProcAddress(h, "DirectSoundCreate")
      |
      +----------> 返回地址,存进局部变量 DirectSoundCreate(我们代码 L122)

DirectSoundCreate(0, &DirectSound, 0)(我们代码 L126)
      |   (1) pcGuidDevice = 0   -> NULL = 用"默认播放设备"
      |   (2) &DirectSound       -> 【输出参数】把新对象的指针写回我的变量
      |   (3) pUnkOuter   = 0    -> COM 聚合用;头文件标了 _Pre_null_,必须 NULL
      +----------> 内部:造一个对象,并在对象偏移 0 处挂上 vtable 指针
      <---------- HRESULT(我们用 SUCCEEDED 判)
      |
      |   * 只有走到这里,DirectSound 才从 0 变成真正的对象
      v
DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)(我们代码 L150)
      +----------> [对象] -> [vtable] -> [+0x30] -> 调 DLL 里的实现
      <---------- HRESULT
```

三个参数逐个对应到我们那一行:`0` / `&DirectSound` / `0`。
第二个参数是**取地址**,因为它的语义是"把结果写进你那个变量里"(输出参数),
所以传进去的是**变量的地址**,不是变量的值。

#### 图 4:顺序不能换(以及 L126 那个 `&&` 是干嘛的)

```
[正确]  Create ---> DirectSound != 0 ---> SetCooperativeLevel ---> OK
[反了]  SetCooperativeLevel ---> DirectSound 还是 0
                                 +-> mov rax,[0]   <- 读地址 0 -> 访问冲突

[没检查 GetProcAddress]
        DirectSoundCreate 变量 = 0  -> call 0 -> 跳进虚空
        => 所以 L126 写成  if (DirectSoundCreate && SUCCEEDED(...))
                            +-- 前半段挡住"函数没取到",后半段挡住"创建失败"
```

这也是为什么"必须先 `Create` 再 `SetCooperativeLevel`"是**硬事实**:
不是约定,而是**没对象就没法调**。

#### 图 5:它在整条流水线上的位置(依赖关系才是完整答案)

```
                        +-- 只有 PRIORITY 级别才允许改主缓冲格式 --+
                        |                                        |
LoadLibraryA("dsound.dll")                                     |
      |                                                        |
GetProcAddress("DirectSoundCreate")                            |
      |                                                        |
DirectSoundCreate(0, &DirectSound, 0)   <- (1)【出生】拿到对象    |
      |                                                        |
      v                                                        v
DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)  <- (2)【定规矩】
      |        级别 = 2(DSSCL_NORMAL=1 / PRIORITY=2 / EXCLUSIVE=3 / WRITEPRIMARY=4)
      |        我们要的就是"有资格改主缓冲格式"这一条
      v
DirectSound->CreateSoundBuffer(主缓冲 DSBCAPS_PRIMARYBUFFER)  <- (3) 只是"改格式的把手"
      +-> PrimaryBuffer->SetFormat(&WaveFormat)   * 这一步依赖 (2) 成功
      v
DirectSound->CreateSoundBuffer(次缓冲, 真缓冲, 384000 字节)    <- (4) 真正写样本的地方(升为全局)
      v
每帧:GetCurrentPosition -> Lock -> 写方波 -> Unlock ->(第一次)Play
```

两条最容易忽略的关系:

1. **`SetCooperativeLevel` 不是"礼节性调用",它是第 (3) 步的前置权限。**
   主缓冲的格式不是谁都能改:先声明"我按 PRIORITY 级别和系统协作",Windows 才放行 `SetFormat`。
   所以代码里这两个调用是**连体**出现的(我们 L150 → 主缓冲那段)。
2. **对象不止一个:`DS->` 和 `PrimaryBuffer->` 是两个不同的对象、两张不同的表。**
   `DirectSound` 用的是 `IDirectSound` 的表(11 格),`PrimaryBuffer` 用的是 `IDirectSoundBuffer` 的表
   (21 格,`SetFormat` 在第 14 格 `0x70`)。**同一个箭头语法,跳的是不同的表** ——
   这正是 2.3.2 说的"每个有虚函数的类型一张表"在实战里的样子。

#### 图 6:三者对照表

| 名字 | 本质 | 类型 / 形式(本机 `dsound.h`) | 从哪来 | 我们代码 |
| --- | --- | --- | --- | --- |
| **`DirectSoundCreate`** | 导出**函数**(平铺,不属于任何对象) | `extern HRESULT WINAPI DirectSoundCreate(LPCGUID, LPDIRECTSOUND *, LPUNKNOWN)` — **L471** | `GetProcAddress` | L122 取地址,L126 调用 |
| **`DirectSound`** | **变量**,装"对象指针" | `LPDIRECTSOUND` = `typedef struct IDirectSound *` — **L166** | 由 `DirectSoundCreate` 的**输出参数**填 | L125 声明,L150 使用 |
| **`SetCooperativeLevel`** | **虚方法**(对象的**行为**) | `IDirectSound` 第 6 格:`STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD)` — **L605** | 对象的 vtable(在 dsound.dll 里) | L150 |
| `DSSCL_PRIORITY` | 常量 = **2** | `#define DSSCL_PRIORITY 0x00000002` — **L2060** | 头文件 | L150 第 2 个参数 |
| `DirectSoundCreate8` | 新版入口,返回 `IDirectSound8 *` | `dsound.h` — **L480** | 同理 | 我们没用 |

#### 图 7:命名为什么这么容易混

```
   IDirectSound         -- 接口"类型"(struct 名)
   LPDIRECTSOUND        -- 指向它的"指针类型"
   DirectSound          -- 我们给变量起的名字(和类型名撞脸,所以最容易晕)
   DirectSoundCreate    -- 创建它的"函数"
   DirectSoundCreate8   -- 创建"升级版接口"的另一个函数
```

一句话记法:**带 `Create` 的是动词(函数),不带的是名词(对象 / 类型)。**
而且 `->` 这个箭头只在**名词(对象)** 后面出现 ——
所以 `DirectSoundCreate->` 这种写法在语法上**根本不可能成立**,这也从形式上印证了图 2。

#### 图 8:变量 → 类型 → 表,三层各管什么

> **答疑 ①(我问的)**:`SetCooperativeLevel` 只是虚方法(对象的**行为**),`DirectSound` 只是变量(装着"对象指针")——
> 那**为什么可以**写 `DirectSound->SetCooperativeLevel(...)`?

**回答:"变量里装什么"和"能不能写箭头",是两件不同层面的事。**
箭头的主角**从来不是变量**,而是**变量指向的那个对象**;变量只是一张写着"对象在哪"的**便签**。

**(1) `->` 的准确含义:先解引用,再调用**

```c
DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY);

(*DirectSound)        .SetCooperativeLevel(Window, DSSCL_PRIORITY);
  ^ 解引用 = "那个对象"   ^ 在【对象】上调用方法
```

`a->b` 就是 `(*a).b` —— **能调方法的主语是"对象",变量只负责回答"对象在哪"**。

**(2) 编译器靠【类型】决定跳第几格,不靠变量**

变量的名字它根本不在乎,甚至**可以没有变量**:

```c
/* 不写变量也完全合法 —— 证明"变量"从来不是必需的一环 */
(Win32CreateDirectSound())->SetCooperativeLevel(Window, DSSCL_PRIORITY);

/* 反过来,类型不对就立刻露馅 */
void *DirectSound = 0;
DirectSound->SetCooperativeLevel(...);   /* ✗ 编译不过:void 没有成员 */
```

⇒ 这就是"它只是个变量"的意思:**变量不携带任何行为**;行为属于**类型**(编译期:第几格)和**表**(运行期:哪张表)。

**(3) 同一行代码的四层阶梯**(每层都在说同一件事)

| 层 | 写法 | 这层揭示了什么 |
| --- | --- | --- |
| **源码** | `DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)` | 我们写的那一行 |
| **① 语法展开** | `(*DirectSound).SetCooperativeLevel(Window, DSSCL_PRIORITY)` | 箭头 = "解引用 + 调用",主语是对象 |
| **② C 风格摊开** | `DirectSound->lpVtbl->SetCooperativeLevel(DirectSound, Window, DSSCL_PRIORITY)` | **头文件 `dsound.h` L620 里真实存在的宏就是这么写的** —— 两次间接,并且把 `DirectSound` 自己**再传一遍**当第一个参数 |
| **③ 汇编** | `mov rax,[DirectSound]` → `mov rax,[rax]` → `rcx=DirectSound`、`rdx=Window`、`r8d=2` → `call qword ptr [rax+30h]` | 我们 exe 里的实测;到这里**变量已经消失了**,只剩它曾经存过的那个值 |

第 ③ 层最能说明问题:变量在 CPU 眼里**根本不存在** —— 它的值被搬进 `rax`(第 1 跳)、搬进 `rcx`(第 1 个参数),然后就没了。
而 ② 与 ③ 是**严格对应**的:宏里的 `(p)` ↔ `rcx = DirectSound`(就是那个 `this`);宏里的 `->lpVtbl->` ↔ `mov rax,[rax]` + `[rax+30h]`;宏里的 `a,b` ↔ `rdx = Window`、`r8d = 2`。

**(4) 便签 / 房间 / 手册**

```
   变量 DirectSound            对象(在 dsound.dll 里)          vtable(在 dsound.dll 里)
   =====================      ===========================    ============================
   便签上写的房间号     --->   房间(第一件东西就是表的地址) --->  本层服务手册(全层共用一本)
                                                               第 6 条 = SetCooperativeLevel
```

- **便签**(变量)没有"叫客房服务"的能力,它只写着"哪个房间";
- 有服务的是**手册**(vtable),手册挂在**房间**(对象)里;
- `->` 读作:"**顺着便签,去那个房间,翻手册**";
- 便签可以撕掉、换一张(变量可以重新赋值、作用域结束就消失),房间和手册不受影响。

**(5) 措辞修正:方法属于"类型",不属于"对象"**

| 名字 | 图 6 里写的 | **更准的说法** |
| --- | --- | --- |
| `DirectSound` | 变量,装"对象指针" | **变量**(运行期:一个地址);**方法不在它身上**,它只是"找到对象"的入口 |
| `SetCooperativeLevel` | 虚方法(对象的**行为**) | **`IDirectSound` 这个类型上的一个方法**(编译期:第 6 格);调用时通过"对象里那个表指针"跳过去 |

严格讲:**方法属于类型,不属于任何一个对象**;对象里只有一条数据 ——"我该用哪张表"。
这也正是"**同一个 `SetCooperativeLevel` 能服务所有 `IDirectSound` 对象**"的原因(见 2.3.2)。

📍 顺带解释了一个细节:`DirectSoundCreate` 的第二个参数必须是 `LPDIRECTSOUND *`(所以传 `&DirectSound`),而**不是** `void **` ——
因为它**决定了我们这个变量以后能写哪些箭头**。签名给的是类型,类型才给出"能调什么"。

#### 图 9:`virtual` 到底重不重要?

> **答疑 ②(我问的)**:既然箭头靠"对象里的那张表"就能走通,那 `SetCooperativeLevel` **是不是虚函数岂不是根本不重要**?

**回答:对"你怎么写"不重要(写法一模一样),对"这段代码怎么跑、能不能调得到"是决定性的。**
`virtual` 在**语法层是隐形的**,在**代码生成层是全部**。

**(1) 直觉对的那一半**

- 我们写 `p->m()` 时,`m` 是不是 `virtual`,**写法完全一样**;
- 编译器数"第几格"靠的是**类型**,和 `virtual` 也无关;
- ⇒ 所以**从"读代码"的角度看,`virtual` 确实是隐形的**。

**(2) 但它决定了生成什么指令**(两种形态在我们 exe 里都有)

| | 非虚(同模块普通函数) | **虚函数** |
| --- | --- | --- |
| 汇编 | `call 0000000140001630` | `mov rax,[rax]` + `call qword ptr [rax+30h]` |
| 目标何时确定 | **链接期**(写死) | **运行期**(由对象里的表指针决定) |
| 谁能改 | 没人能改 | **每个对象各自** |
| 同一段调用代码能否跨实现复用 | 不能 | 能 |

一个字之差,换来的是"**这条调用能不能跟着对象走**"。

**(3) ★ 关键:没有 `virtual`,我们连这个方法都调不到**

本机 `C:\Windows\System32\dsound.dll` 的**全部导出**只有 12 个名字(`dumpbin /exports` 实测):

```
DirectSoundCreate            DirectSoundCreate8
DirectSoundCaptureCreate     DirectSoundCaptureCreate8
DirectSoundEnumerateA/W      DirectSoundCaptureEnumerateA/W
DirectSoundFullDuplexCreate  GetDeviceID
DllCanUnloadNow              DllGetClassObject
```

| 名字 | 导出情况 |
| --- | --- |
| `SetCooperativeLevel` | **未导出** |
| `CreateSoundBuffer` | **未导出** |
| `GetCurrentPosition` / `Lock` / `Unlock` / `Play` | **未导出** |
| `QueryInterface` | **未导出** |

⇒ 我们**没有第二条路**能拿到 `SetCooperativeLevel`:

- 不能 `GetProcAddress` —— **DLL 里根本没有这个名字**;
- 也没有导入库条目 —— `dsound.lib` 里只有那几个平铺的 `Create` 函数,方法一个都没有。

**唯一的入口就是"顺着对象里的表跳过去",而那张表就是 `virtual` 造出来的。**
所以对这个 API 来说,`virtual` 不是"更好的设计",而是"**唯一可行的设计**"。

**(4) 类型和虚函数各管什么(缺一不可)**

| 谁 | 负责回答 | 缺了会怎样 |
| --- | --- | --- |
| **类型**(`IDirectSound`) | **第几格** → `0x30` | 不知道偏移,编译不过 |
| **虚函数机制 + 对象里的表指针** | **哪张表**(哪个实现) | 不知道跳谁,只能写死一个实现 |
| 变量 | 只是"地址存在哪" | 无所谓(甚至可以没有变量) |

一句话:**类型给"格子号",对象给"哪本手册"。**

**(5) 那什么时候 `virtual` 真的不重要?**

- 如果这些方法**就在同一个模块里、且只有一个实现**,非虚完全够用(一条 `call 常量`)——
  那时 `virtual` 唯一的好处只是"将来可替换";
- 但我们的处境是三个条件同时成立:**实现在 DLL 里** + **可能有多个实现**(`DirectSound` / `DirectSound8`)+ **不能靠名字取**
  ⇒ **非虚方案在这个场景下压根不存在**。

这也回头解释了 2.3.1 那张四分表:**第一行**(同模块普通函数)不需要表,**第四行**(虚函数)才需要 ——
不是虚函数"更高级",而是它专门解决"**目标是运行期才知道**"这一类问题,而我们碰上的正好是这一类。

**(6) 对后面写游戏的实际意义**

1. 我们现在每帧调的 `Lock` / `Unlock` / `Play` **全是虚调用** —— 一天几十次,代价忽略不计(见 2.3.6);
2. 但**以后自己写实体 / 物理系统时**,别把"每帧跑几万次"的东西做成虚函数,那才是 2.3.6 说的热路径问题
   (那时宁可手写函数指针,也就是回到 XInput 那种做法);
3. 反过来,凡是"**接口 + 可替换实现**"的地方,虚函数是**唯一能跨 DLL 又不靠名字**的手段 ——
   这就是为什么整个 COM 全是 `virtual`。

#### 一个用词的精度:它其实不是"句柄"

我们 L127 的注释写的是:

> 当我们拿回指向 DirectSound 的句柄时,DirectSound 拥有一个指向 DirectSound DLL 内部虚表的指针

**后半句完全正确**(这就是 2.3.2 那张布局图)。只有"**句柄**"这个词可以再拧准一点:

| | 句柄(`HWND` / `HMODULE` / XInput 的索引) | **接口指针**(`LPDIRECTSOUND`) |
| --- | --- | --- |
| 本质 | **整数编号** | **内存地址** |
| 用法 | 交回给系统,由系统查表找到对象 | 直接顺着它去读 vtable |
| 拿不到会怎样 | 传错的编号 → 系统查不到 → 失败返回 | 野指针 → 访问冲突 |

📍 **这正是"为什么 DirectSound 不能像 XInput 那样用一个整数索引"的原因** ——
我们要的不是"编号",而是**一个里面装着表指针的对象**。

#### 🟡 不确定的地方(照实标注)

- "必须在**创建任何缓冲之前**调 `SetCooperativeLevel`"这条,是**视频里他这么讲的**,
  我们的代码也是这个顺序;但我手头没有离线文档原文可核(网上也没找到对应的原生 API 页),
  所以"不这么做会怎样"的**严格性边界我不敢替它保证**。
  确定的是前一条:**先 Create 才能调**,这是语言/接口层面的硬约束。
- `DSSCL_PRIORITY` 除了"允许改主缓冲格式"之外还有没有别的副作用,PRIORITY 与 NORMAL 的细微差别
  (前台/后台行为等),这一集没讲,我也没在离线资料里核过 🟡。

#### 📍 对我们的意义

1. **读 API 时先问"它是名词还是动词"**:`DirectSoundCreate` 一类是入口(靠 `GetProcAddress`),
   其余都是对象上的方法(靠 `->`)。这条经验对后面所有 COM 风格 API 通用。
2. **顺序即依赖**:`Create → SetCooperativeLevel → 主缓冲 → SetFormat → 次缓冲` 是一条链,
   前一步不成功,后一步没有意义 —— 所以我们那 6 个 `OutputDebugStringA` 诊断要**一路挂下去**,
   而不是只报最后一个失败。
3. **"对象"是可以有多个、且各自带表的**:`DirectSound` 和 `PrimaryBuffer` 就是两个。
   明天写 `Lock` / `Unlock` 时,`->` 前面的东西换成了 `GlobalSecondaryBuffer`,
   那是一张**第三张**表(`IDirectSoundBuffer` 的另一个对象),概念上完全一样。

### 2.5 他用调试器**当场看汇编**(这一段的实证价值最高)

| 情形 | 汇编长什么样(他的观察) | 时间 |
|---|---|---|
| 普通函数(我们自己写的) | **`call <常量地址>`** —— 链接器把地址写死 | `00:11:12` |
| 虚函数(`SetCooperativeLevel`) | **先 `mov` 出对象 → 再 `mov` 出 vtable 指针 → 再 `mov` 出函数地址 → `call rax`** | `00:11:46` |

> [!note]
> 这张表是他当时口述的**简化版**;精确版(把"别的 DLL 的导出函数 / 我们自己手写的函数指针"也区分开)见 **2.3.1**。

他还补了两点:

1. **那张 vtable 在哪?** 在被加载的 **DLL 里** —— `00:09:16` "the vtable got mapped into memory when we loaded the dll";
   所以 `DirectSoundCreate` 回给我们的对象里,那个 vtable 指针**指向 dsound.dll 内部的表**;
2. **CPU 不在乎你写什么语言**:`00:10:20` "everything gets translated to CPU… languages make things simpler for the programmer, they do not make things any simpler for the CPU"。
3. 他还很诚实地标了不确定:`00:12:52` "I'm not the best person at Assembly Language… I'm just basically assuming that this is roughly what it has to do" 🟡。

> [!tip]
> **对我们的意义**:为什么 DirectSound 的 API 要用 `对象->方法()` 这种形式,而 XInput 是"取一个函数指针直接调"?
> 因为 DirectSound 是 **COM** 接口:`DirectSoundCreate` 给你的**不是一个句柄,而是一个带 vtable 的对象**;
> 之后那些方法**不需要再 `GetProcAddress`**,直接从这个对象里"跳"过去。
> 我们**不需要懂 COM**,只要知道"这个箭头 = 两次间接寻址"就够了 —— 而且这一集他不会用任何 C++ 特性。

---

## 三、回顾昨天的初始化(以及 48kHz 是怎么定的)

- 主缓冲的用途:`00:13:33` "we are basically asking DirectSound for a handle to the primary buffer and that primary buffer we are using to **set the output format**" —— 立体声 / 2 声道 / **16 位** / **48 kHz**。
- ⭐ **48kHz 的来历**(这条很有意思,`00:14:00`):
  > "I asked **Jeff Roberts**, who maintained the **Miles Sound System** for a very long time, what was the most common native format for sound output — and he said **48 kHz**."
  他也说 44.1kHz(CD 音质)完全可以,所有声卡都支持;差别只在于**会不会被重采样**。
- **内核混音器**(`00:14:24`):"we can't write into the primary buffer, those days are over" ——
  所有程序各自往**次缓冲**写,**内核把它们混起来**再送声卡;他打了个漂亮的类比:
  > "**kind of like we no longer have direct write privilege to the graphics card**"

---

## 四、★ 把次缓冲指针升为全局(附他关于全局变量的规矩)

`00:15:02` 他把 secondary buffer 从局部**提升成全局**,理由:这是**每帧都要用**的东西(写声音)。

顺带讲了他对全局变量的态度(`00:15:02` 起),这三条**判据**值得抄下来:

> 1. **你要明白它为什么是全局**;
> 2. **它应该一直保持全局**;
> 3. **它只会有唯一一个**;
> —— "**if any of those things aren't true, don't do it**"

他还说:平台层里全局变量多一点其实问题不大(**因为代码高度隔离**),"but be aware of what you make global, **don't do it willy-nilly**"(别随手就加)。

---

## 五、★ 方波的数学(这一集的核心新知识)

### 5.1 为什么先放方波

`00:15:59` 他不想一上来就加载 wav(那是后面的事),先用**最简单的波形**证明"能把声音送出去":

| 波形 | 形状 | 代码难度 |
|---|---|---|
| **方波(square)** | 高电平一段 + 低电平一段,**中间什么都没有** | ★ 最简单 |
| 正弦(sine) | 平滑来回 | 要 **浮点**(他说明天讲) |
| 三角(triangle) | 线性上去再下来 | 中等 |
| 锯齿(sawtooth) | 斜上去然后瞬间掉回 | 中等 |

方波就是他说的 `00:16:52` "just a full-on oscillation"。

### 5.2 Hz = 每秒多少**周期**

`00:30:53` 他现场解释:

- **一个周期(cycle)= 波形完整重复一次**(比如从一个峰值到下一个峰值);
- **Hz = cycles per second**;
- 所以他要知道"听起来像什么"就得先知道频率 —— 他挑了**中音 C**;
- 中音 C 的真实频率是 **261 Hz** 左右,他直接取 **256**(`00:32:1x`:"I want a power of two just because I'm a programmer like that"),承认"不精确,但够用"。

### 5.3 从频率倒推"一个周期占多少采样"

这是本节最关键的一步(他说得很清楚,`00:32:45`):

```
samplesPerSecond = 48000          (我们向缓冲申请的就是这个)
toneHz           = 256            (想要多少周期/秒)
一个周期占的采样数 = samplesPerSecond / toneHz = 48000 / 256 ≈ 187 个采样
halfSquareWavePeriod = 一个周期 / 2   ≈ 93
```

### 5.4 每个采样输出 +A 还是 −A:用"第几个半周期"的奇偶

他的写法(`01:09:00` 附近解释):

```c
/* 走到了第几个半周期 */
uint32 HalfPeriodIndex = RunningSampleIndex / HalfSquareWavePeriod;
/* 偶数 → 正、奇数 → 负(或反过来,相位无所谓) */
int16 SampleValue = ((HalfPeriodIndex % 2) == 0) ? ToneVolume : -ToneVolume;
```

- `RunningSampleIndex / HalfSquareWavePeriod` 得到的是一个**逐个递增的整数**(0,1,2,3…)—— 他在图里把它讲成"我现在在第几个桶里";
- 再 `% 2` 把它压成 **0 / 1**,于是波形**一正一负地跳**;
- **弹幕问"能不能直接看最低位?"** —— `01:09:54` 他说 **可以,而且 `& 1` 和 `% 2` 在这里是同一件事**:
  🟡 "I believe that is exactly what the optimizer would do here"(他没确保优化器一定这么干,因为"mod 可能有自己的规则");
  但他**故意写 `% 2`**,因为"那是你在逻辑上想做的事" —— 要是真在乎性能就写 `& 1`。

### 5.5 音量,以及 16 位样本的范围

- 样本是 **`int16`**,范围 **−32768 ~ +32767**(✅ 头文件实测:`INT16_MIN = -32768`、`INT16_MAX = 32767`);
  他原话 `00:29:05`:"this is a 16-bit value, so **32,000 is the maximum** it could be… 65536 divided by two";
- 他一开始随手写了个 **60000** —— **当场把自己耳朵震了**(见第十节),于是把音量做成参数 `ToneVolume`,一路降到 6000 → 4000 → **3000**。

---

## 六、★ `Lock` 为什么可能返回**两块**区域

`00:18:19` 他重新翻出 `IDirectSoundBuffer::Lock` 的文档,又画了一遍环形缓冲的图。

| 关键点 | 说明 |
|---|---|
| 环形缓冲 | 播到末尾**自动回到开头**继续播(`00:18:41`) |
| 为什么要 Lock | 这是**系统的内存**,Windows 要知道我们什么时候在写它 ⇒ 先"锁",写完**必须** Unlock |
| **可能返回两块** | 如果你要写的长度**跨过了缓冲区末尾**,Lock 会把"尾巴那段"和"开头那段"**分别**给你 ⇒ `ppvAudioPtr1/2` 两个指针 + 两个长度 |
| 也可能只有一块 | 如果请求的区域不跨末尾,`ppvAudioPtr2` 就是 NULL |

文档原话(他在 `00:40:01` 念的):**"after writing data to the pointer returned by this method, the application must immediately call Unlock"**
⇒ **Unlock 不是可选的**。(记住这句,第十节就是栽在它上面。)

签名(✅ `um\dsound.h` L713 起):

```c
STDMETHOD(Lock)(THIS_ DWORD dwOffset, DWORD dwBytes,
                LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1,
                LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags) PURE;
STDMETHOD(Unlock)(THIS_ LPVOID pvAudioPtr1, DWORD dwAudioBytes1,
                  LPVOID pvAudioPtr2, DWORD dwAudioBytes2) PURE;
```

> 可用的 flags:✅ `DSBLOCK_FROMWRITECURSOR = 0x1`、`DSBLOCK_ENTIREBUFFER = 0x2` —— **他两个都没用**(传 0)。

---

## 七、★ `GetCurrentPosition`:play cursor 与 write cursor

`00:36:05` 他去找"怎么知道该往哪儿写",找到了 `IDirectSoundBuffer::GetCurrentPosition`:

```c
STDMETHOD(GetCurrentPosition)(THIS_ LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor) PURE;   /* ✅ L706 */
```

### 7.1 为什么不能"就地写"

他画的图 + 解释(`00:38:33` 起):

- **play cursor**:硬件(或内核混音器)**正在读**的位置;
- 这件事**与我们无关地一直在发生** —— 可能在**另一个核上并发**,我们可能**随时被打断**;
- 所以**直接从播放位置开始写是不安全的**:等我们写完,播放头**可能已经跑到我们正在写的区域里**,于是它播出**写了一半的数据**(杂音);
- 解决的机制:**还有一个 write cursor,它领先于 play cursor**;
  write cursor 告诉我们的只是"**从这儿往后写是安全的**"。

### 7.2 但结论很反直觉:**我们不关心 write cursor**

`00:40:01` 他现场查文档确认:**`Lock` 不会移动 write cursor**。于是他的结论(`00:40:44`):

> "**the write cursor is not actually going to be something we need to know about** ——
> it's really the **play cursor** we're going to have to see, because we have to look to see where we can fill."

也就是说:**我们自己记住写到哪了**(下一节的 `runningSampleIndex`),然后**一直写到 play cursor 之前**为止。
`00:41:07` 他还安慰观众:等把这套环形缓冲的东西**抽象掉**以后,以后输出声音会简单得多。

---

## 八、★ 写缓冲的完整算法

### 8.1 用**永远递增**的采样索引,再"折"回环形缓冲

| 变量 | 含义 | 为什么这么设计 |
|---|---|---|
| `RunningSampleIndex` | 从 0 开始**永远递增**、表示"我一共写了多少个采样帧" | **必须用无符号**:`00:42:22` "we want it to be an unsigned integer because if it ever wraps over 4 billion we want it to go back to zero, not to a negative number" |
| `ByteToLock` | 这次要**锁**的位置(字节) | `= (RunningSampleIndex * BytesPerSample) % SecondaryBufferSize` |
| `BytesToWrite` | 这次要**写**多少字节 | 见下面两种情形 |

⭐ **mod 的作用**(他专门讲了两遍,`01:05:22` 与 `01:07:13`):
`绝对值 % 缓冲大小` = **把"绝对索引"变成"在当前这一圈里的相对位置"** ——
商的整数部分 = **第几圈**,余数 = **这一圈里的第几个字节**。这正是"把一条无限长的直线折进一个环"。

### 8.2 `BytesToWrite`:两种情况

```
情形一:ByteToLock > PlayCursor(要写的位置在播放头【后面】,得绕过末尾)
        BytesToWrite = (SecondaryBufferSize - ByteToLock) + PlayCursor
情形二:ByteToLock <= PlayCursor(播放头在前面,一口气写完)
        BytesToWrite = PlayCursor - ByteToLock
```

他反复强调"**我们不去写超过 play cursor 的地方**"(`00:44:33`:"we don't want to write past the play cursor")。
图在 `00:45:56` 起画了两遍(case one / case two)。

> [!note]
> 这两行就是"环形缓冲最容易写错的地方" —— 他在 `01:21:57` 亲口说:
> "**I always write it wrong the first time**",因为里面偏移量太多、还有"该用 sample index 还是 byte index"这种坑。

### 8.3 写样本:两个 region、左右声道交错

```c
int16 *SampleOut = (int16 *)Region1;               /* Lock 给的区域,按 int16 看待 */
for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
{
    int16 SampleValue = ...;                       /* 见 §5.4 */
    *SampleOut++ = SampleValue;                    /* 左声道 */
    *SampleOut++ = SampleValue;                    /* 右声道(一样) */
    ++RunningSampleIndex;                          /* ← 每次输出一个采样帧就前进一格 */
}
/* region 2 一模一样再来一遍 */
```

要点:

- **一个"采样"在这里指"左 16 位 + 右 16 位"**(交错存放),所以**永远要成对写**;
  `00:24:23` "16 bits for the left channel, 16 bits for the right channel… we can't output anything less than that";
- `BytesPerSample = 4`(2 声道 × 16 位 ÷ 8);
- region 的长度是**字节**,循环要用**采样数**:`RegionSampleCount = RegionSize / BytesPerSample`;
- 🟡 他提到这里**应该加 assert**(保证长度是采样大小的整数倍),但说"我还没跟你讲过 assert 是什么,以后再讲,不想现在乱塞"(`00:25:12`);
- 两个 region 的循环**完全一样**,他说(`00:59:xx` 预告 / `01:03:04` 又说一次):下次会**合并成一个**。

### 8.4 顺带:他的"压缩导向编程"

`00:50:48` 他说了一段方法论,值得记:

> "**compression-oriented programming is the best way to do things**… 我就是**一直写当下最直白的那个东西**,不过度思考,
> **模式会自己浮现** —— 你能看出来这两个循环是同一个、能看出来以后要抽什么公共部分。"

---

## 九、`Play(0, 0, DSBPLAY_LOOPING)`:让它响

`00:56:13` 他发现"只填缓冲不会响"—— 还得**告诉缓冲开始播放**:

```c
GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
```

✅ 签名 `STDMETHOD(Play)(THIS_ DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)`(L716),三个参数:

| 参数 | 他传的 | 含义 |
|---|---|---|
| `dwReserved1` | **0** | **保留位,必须传 0**(`00:56:43` 他念文档确认) |
| `dwPriority` | **0** | 优先级 —— 只播一个声音时**无所谓**(多个缓冲混音时才有意义) |
| `dwFlags` | **`DSBPLAY_LOOPING`(0x1)** | **循环播放**:我们就要它一直播下去 |

---

## 十、★ 现场:响了 → 太吵 → 咔哒声 → 忘了 `Unlock`

| 时间 | 发生了什么 |
|---|---|
| `00:57:40` | **"wow, well the square wave works ladies and gentlemen"** —— 真的响了(他自己都被音量吓了一跳) |
| `00:57:55` | 音量太大 → 把 `ToneVolume` **做成参数**,从 60000 一路降到 6000 → 4000 → **3000** |
| `00:58:32` | 他说"**我还能听到咔哒声(clicking),我觉得还有 bug**" |
| `00:59:27` | ⭐ **弹幕提醒:"是不是忘了 `Unlock`?"** —— "yes that's right, **we forgot that as well**" |
| `01:00:08` | 补上 `Unlock` 之后:**"no, that actually does sound right"** ✅ |
| `01:00:29` | 但他仍不放心:"我还要**更严格地验证缓冲里的内容**",并说**明天专门调试**这块 |
| `01:21:57` | Q&A 里他承认:**环形缓冲的代码他"第一次写总是错的"**,所以必须**画图 + 逐项核对**"我们是不是真的写到了以为的地方" |

> [!note]
> **两个 bug 都很典型**:① 音量没参数化,直接震耳朵;② **`Lock` 之后必须 `Unlock`**(文档明确写"must immediately call Unlock")——
> 不 Unlock,DirectSound 会"confused",表现就是我们听到的周期性咔哒声。

---

## 十一、Q&A 前他当场补的两处(和"启动一秒后才出声"有关)

Q&A 里有人问:**"为什么声音要等大约一秒才开始?"**(`01:18:14`)他借这个问题改了两处代码:

### 11.1 顺序错了:**应该先填满,再 `Play`**

原来的顺序是"**先 Play,再每帧往里填**" ⇒ 播放头从一个**还没写过的缓冲**开始播 ⇒
它得**先播一遍空的/垃圾的**,等绕回来才有我们写的数据 ⇒ 于是有"**一秒的空白**"。

他的修法(用个标志位):

```c
if (!SoundIsPlaying)                 /* 第一帧:先把整个缓冲填满 */
{
    ... 填缓冲 ...
    GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
    SoundIsPlaying = true;
}
```

### 11.2 处理 `PlayCursor == ByteToLock` 的边界情形

第一帧时两个游标**都是 0**,于是 `BytesToWrite` 算出来是 **0** —— 等于**什么都不写**。
他的处理(`01:19:59`):**把这种情况当成"填满整个缓冲"**:

> "if they're equal… we'll say that the `BytesToWrite` is **the whole buffer**" ——
> 并注明"**正常情况下它们永远不该相等**(我们总要写在前面)",只有**刚启动**时会遇到。

改完之后:`01:21:08` "**the sound now starts basically instantaneously**" ✅

---

## 十二、Q&A 要点(从 `01:10:32` 到 `01:31:15`)

> 这一集的 Q&A 里,**延迟**那一段他讲了近 8 分钟,是所有"游戏音频"问题的总纲。按主题整理如下。

### 12.1 音频 API 与兼容性

| 问题 | 他怎么说 |
|---|---|
| **为什么不用 XAudio2?** | `01:11:23`:**因为它不支持老平台**。他要在**默认安装的 Windows XP** 上跑得起来 —— 这是一个**练习**:"让你学会**在不强迫用户装一堆驱动**的前提下把程序装上、跑起来"。XP 现在不到 10% 的机器,他说"我不是在逼谁兼容 XP,你完全可以从 Vista 起步" |
| **XP 上到底有没有 XAudio2?** | `01:14:12`:**没有**(默认安装没有);你可以在 XP 上装,但那就变成"强迫用户装驱动"。他还标了不确定:🟡 "I'm 99% sure… if I'm wrong someone can correct me" |
| **DirectSound 值不值得用?** | `01:12:14`:我们用它在 XP 上跑得好好的,**"there really isn't much reason to go use XAudio2"** —— 除非你在乎更低的延迟 |
| **会不会把播放声音抽成单独函数?** | `01:12:47`:会,**而且不止一个** |
| **会不会拆分成多个文件?** | `01:17:17`:**当然会** |
| **要不要做混响/滤波器?** | `01:16:56`:可以做,但他**自己很少用**(尤其 2D 游戏里意义不大);真要做得漂亮,他愿意去问 **Jeff Roberts** |
| **次缓冲为什么不起个更好懂的名字?** | `01:13:49`:以后可能改;但**不重要** —— 平台层代码就这么点,而且它**永远只活在 `win32_handmade.cpp` 里**,游戏层**根本看不到**这些变量 |

### 12.2 输入与键位(顺带问到 Day 006 的选择)

| 问题 | 他怎么说 |
|---|---|
| **为什么键盘用事件而不是轮询?** | `01:14:50`:**"没有别的特别原因,就是事件不会丢"** —— 操作系统已经在内部收集键盘事件了;我们**主动去 poll 反而可能漏掉**某个按键事件 |
| **会不会支持按键重映射?** | `01:15:26`:他**不太喜欢可重映射的键**(个人偏好);但**一定会支持多种键位布局** —— 比如法国的 AZERTY(W/A/S/D 位置不同) |

### 12.3 ★ 延迟:他讲了近 8 分钟(本集最"值钱"的问答)

**问题一**:"更大的主缓冲是不是意味着更长的延迟?"(`01:15:51`)

> **不是。** 他先说"主缓冲的大小和延迟**毫无关系**",接着指出**更根本的问题**:
> 我们**根本没设主缓冲的大小**(主缓冲只是个改格式的把手,大小是 0)——
> 所以他反问观众:"你问的其实是**次缓冲**吧?"
> 然后回到 §8 的图:**延迟取决于"你把写指针放在播放头前面多远",而不是缓冲总长**。

**问题二**:"游戏能接受的延迟是多少?**应该领先读指针多少**?"(`01:23:07`)

他给了一个**非常坦率**的答案:**"it depends —— 而且在有些平台上根本算不出来"**:

| 平台 | 能不能精确同步 |
|---|---|
| **macOS / Core Audio** | 可以**硬同步**:系统会告诉你"你的视频帧**什么时候**从 HDMI 口出去",于是你能把音频也对齐到那个时刻 |
| **Windows(DirectSound,至少 XP~7)** | **有一段说不清的 slop**。`01:27:03`:"we don't really have any way of telling Windows to **place the sound buffer so that it perfectly coincides with when the picture flips**" |

他还画了时间轴解释"理想情况":第 N 帧的**画面**和它的**那段声音**应该**同时**出现;
"应该有多少延迟"= **从我们写完声音到那一帧真的显示出来之间的时间** —— 但这个时间**拿不到**。
所以:

- 这就是为什么今天的游戏**常带"延迟校准器"**(他举例 **Guitar Hero** 那种要你跟着节拍按键的游戏);
- 而且**就算你把 Windows 这一层搞对了**,后面还有**你控制不了的延迟**:
  **HDTV 会缓存帧**、**功放/回音壁自己会加延迟**、音频和视频走的还可能是**不同的路径** ——
  `01:28:29` "it's **a nightmare**"。

**然后是他著名的 "no poop sauce" 提案**(`01:29:20` 起,纯吐槽但很有代表性):

> 他说自己为这件事**抱怨了十年**:他希望
> ① **OpenGL(图形驱动)直接把声音缓冲和帧一起送出去**;
> ② 电视/显示器上贴一个**认证标志** —— 他画的是"**一坨便便加一道斜杠**" = **no poop sauce** ——
> 保证"这台设备**不额外加任何延迟**";
> 这样玩家就知道:买带这个标志的电视/显示器,**画面和声音真的同时出来**。
> `01:30:50` "I have advocated this for many years in the industry — **nobody cared**"。
> 最后他给出"答案":**"how much latency you want — nobody knows, it's completely unknown"**。

> [!tip]
> **对我们的意义**:这条和 Day 007 §6.6(音画同步)是同一件事,但这里给出了**为什么做不到**的完整理由链:
> Windows 不告诉你帧什么时候翻 → 电视/功放再各自加延迟 → 所以只能**估**,并且**别把"完美同步"写进需求**。

### 12.4 代码与风格

| 问题 | 他怎么说 |
|---|---|
| **`/` 和 `%` 的真值表**(弹幕纠正他"1 mod 2 = 1") | `01:12:53`:他确认自己写对了:`1/2 = 0` 余 `1`;除法每两个数才涨 1(**0 0 1 1 2 2…**),而 mod 是 **0 1 0 1…** |
| **两个 region 的循环** | `01:03:04`:下次**合并成一个**(它们完全一样) |
| **mod 与 `& 1`** | `01:09:54`:等价,优化器大概也是这么做的;但他写 mod 因为**那是逻辑意图**;真在乎性能时再写 `& 1` |
| **"我能不能不要这么绕?"** | 他的方法论:**先写最直白的**,让模式浮现(§8.4) |

### 12.5 杂项

- 日程:`01:11:09` **每周五**都会是"欧洲友好"的时间(今天上午 11 点是因为感恩节);
- 明天:`01:21:08` **继续调试声音**,并且**改用正弦波**(那时会正式引入**浮点数**);
- 论坛:`01:32:02` 今天或明天上线;
- 他这集一直重感冒(多处道歉,`01:21:5x` 尤其明显)。

---

## 十三、和我们代码的关系 / 下一步

我们现在的进度(参考 `explain\!AI笔记\项目状态与交接.md`):**Day 007 的 DirectSound 初始化已经打通**
(`LoadLibraryA("dsound.dll")` → `DirectSoundCreate` → `SetCooperativeLevel` → 主缓冲 + `SetFormat` → 次缓冲 + `GlobalSecondaryBuffer`)。这一集要接着加的是:

| # | 要写的东西 | 关键点 |
|---|---|---|
| 1 | **每帧取 `PlayCursor`** | `GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)`;文档说两个参数都可以传 NULL,但我们要 play cursor |
| 2 | **`RunningSampleIndex`(全局、无符号)** | 永远递增;`ByteToLock = (RunningSampleIndex * BytesPerSample) % SecondaryBufferSize` |
| 3 | **算 `BytesToWrite` 的两种情形** | `ByteToLock > PlayCursor` ⇒ `(Size - ByteToLock) + PlayCursor`;否则 ⇒ `PlayCursor - ByteToLock`;**特殊情况:两者相等 ⇒ 填满整个缓冲** |
| 4 | **`Lock` → 写两个 region → `Unlock`** | ⚠️ **`Unlock` 绝不能忘**(今天就是栽在这);region 长度是字节,要 `/ BytesPerSample` 换成采样数 |
| 5 | **方波样本** | `((RunningSampleIndex / HalfSquareWavePeriod) % 2) ? -ToneVolume : ToneVolume`,左右各写一个 `int16` |
| 6 | **`ToneVolume` 取小一点** | 他最后用 **3000**;别学他一开始写 60000 |
| 7 | **第一帧:先填满再 `Play(0, 0, DSBPLAY_LOOPING)`** | 用 `SoundIsPlaying` 标志;这样**启动即出声** |
| 8 | 顺带 | 两个 region 循环**以后合并**;`BytesPerSample` 现在写死 4(2 声道 × 16 位 ÷ 8) |

**还欠着的旧账**(和本集无关但别忘了):

- XInput 只做两级(1_4 → 1_3),可选加第三级 `xinput9_1_0.dll`;
- `VirtualAlloc` 补 `MEM_RESERVE`(Day 007 Q&A 的更正);
- 每帧轮询 4 个手柄索引会 stall(等性能阶段改 HID 通知)。

---

## 十四、★ 音频系统全景:我们仓库现在这份代码是怎么实现的

> [!info] 本节与前面的分工
> §二 ~ §十三 讲的是**这一集**(Casey 怎么想、为什么这么做);本节讲的是**我们仓库当前这份代码**
> (`HandMade\code\win32_handmade.cpp`,**758 行**,2026-09-30)的**完整链路、真实行号与数据流**。
> 每一小节都指回前面讲"为什么"的地方 —— **本节不重复那些推理**,只把它们串成一条线。
>
> **证据**:行号来自直接读源码(2026-09-30);`[rax+0x30]` 这类来自 `dumpbin /disasm` 实测;
> 常量与签名核对过本机 `dsound.h`。🟡 = 推断。

### 14.1 一张图:从"想出声"到"耳朵听到"

```
   我们的代码                                DirectSound(dsound.dll)              硬件
   ─────────                              ────────────────────────             ──────
   ① 一次性搭建(Win32InitSound,L119-215)
      LoadLibraryA("dsound.dll")  ───────▶ 把 DLL 载入进程(代码段 + 数据段)
      GetProcAddress(…,"DirectSoundCreate")▶ 拿到函数地址 —— 它不在导入表里!(Day 007 附录 E)
      DirectSoundCreate(0,&DS,0)  ───────▶ 造出 IDirectSound 对象(对象首 8 字节 = vtable 指针)
      DS->SetCooperativeLevel(…,PRIORITY)─▶ call [rax+0x30] = 表里第 6 格(§2.3.4 的实测)
      DS->CreateSoundBuffer(主缓冲) ──────▶ 拿到"改格式的把手"(PRIMARYBUFFER / 大小 0 / 格式 NULL)
        PrimaryBuffer->SetFormat(…)  ─────▶ call [rax+0x70]:48000 / 16 位 / 2 声道
      DS->CreateSoundBuffer(次缓冲) ──────▶ 384000 字节的真缓冲 ⇒ GlobalSecondaryBuffer(全局,L82)
                                                │
   ② 每帧(主循环,L678-737)                     │  ★ 这块内存由我们自己写
      GetCurrentPosition(&Play,&Write) ───▶ 播放头现在在哪(字节)
      算 ByteToLock / BytesToWrite
      Lock ───────────────────────────────▶ "借"给我们这段内存(跨末尾时给两块)
      写方波(左 16 位 + 右 16 位)× N  ◀─────┘
      Unlock ─────────────────────────────▶ 还回去(⚠️ 不还 = 咔哒声,§十)
      首次:Play(0,0,DSBPLAY_LOOPING) ─────▶ 播放头开始走 ⇒ 有声音了
                                                │
                                                ▼  声卡以 48000 × 4 = 192000 字节/秒 消费这块环形内存
```

### 14.2 初始化:七步,每步失败都只"降级"不崩(L119-215 / §三 / Day 007)

| # | 做什么 | 代码行 | 失败时 | 为什么这么写 |
|---|---|---|---|---|
| 1 | 加载 `dsound.dll` | L122 | 打印"running without sound",**继续跑** | 缺音频不该让游戏打不开(与 ADR 0004 同一哲学) |
| 2 | `GetProcAddress("DirectSoundCreate")` | L126 | 下一行 `if (DirectSoundCreate && …)` 挡住空指针 | 导出函数要**按名字**取;它不在导入表里 |
| 3 | `DirectSoundCreate(0,&DS,0)` | L130 | L208 打印,不崩 | ①默认设备 ②输出参数 ③COM 聚合必须 NULL(§2.4 图 3) |
| 4 | `DS->SetCooperativeLevel(Window, DSSCL_PRIORITY)` | L154 | L156 的 `SUCCEEDED` 直接跳过整个主缓冲段 | PRIORITY = 允许我们改主缓冲格式(§2.4 图 5) |
| 5 | 主缓冲 `CreateSoundBuffer` + `SetFormat` | L161-173 | L182 / L178 打印 | 主缓冲**不是缓冲**,是"拿声卡句柄改格式";失败只是格式没定,不致命 |
| 6 | 次缓冲 `CreateSoundBuffer(…,&GlobalSecondaryBuffer,0)` | L190-196 | L203 打印 | 真正的数据区;`dwBufferBytes = BufferSize`(**外部传入**,见 14.4) |
| 7 | (隐含)把大小/格式相关量留成全局 | L82-90 | —— | 每帧要用 ⇒ 活不过函数返回就得是全局(§四) |

**五处诊断字符串**(用 `cdb -g -G` 能直接抓到,见 14.10):L178 / L182 / L203 / L208 / L213。

> [!warning] 这一步最容易踩的两个坑
> 1. **`WAVEFORMATEX` 的字段顺序**:必须先算 `nBlockAlign`(L147)再算 `nAvgBytesPerSec`(L149)——
>    结构体里声明顺序是**反的**,写反了只会得到 `E_INVALIDARG`,不告诉你哪错了(Day 007 §13.1);
> 2. **`LoadLibraryA("Dsound.lib")` 是错的**:`.lib` 是给链接器看的**导入库**,里面没有可执行代码,
>    `LoadLibrary` 只接受 `.dll`(Day 007 附录 E 有实测)。

### 14.3 格式:为什么是 48000 / 16 位 / 2 声道(L553-554 / §三)

| 参数 | 值 | 我们的代码里 |
|---|---|---|
| 采样率 | **48000 Hz** | `SamplesPerSecond`(L553)—— `00:14:00` 他问了 Jeff Roberts(Miles Sound System),答案就是 48000 |
| 位深 | **16 位** | `wBitsPerSample = 16` ⇒ 一个样本是 `int16`(±32767) |
| 声道 | **2(立体声)** | 交错存放:`L16 R16 L16 R16 …` |
| 每帧字节 | **4** | `BytesPerSample = sizeof(int16) * 2`(L85)= `nBlockAlign`(L147) |
| 每秒字节 | **192000** | `nAvgBytesPerSec`(L149)= 48000 × 4 |

📌 **这一组数字决定了后面所有的单位换算**,所以它们被抽成**变量**而不是散落的常数 ——
这正是 §五 里那句 "a complete set of definitions for our sound system" 的意思。

### 14.4 缓冲的形态:环形 + 交错(L559 / §8.1)

```
   GlobalSecondaryBuffer:384000 字节 = 2 秒 = 96000 个采样帧(每帧 4 字节)
   帧 0          帧 1          帧 2                     …        帧 95999    绕回帧 0
  ┌──────────┬──────────┬──────────┬─────   …   ─────┬──────────┐
  │ L16 R16  │ L16 R16  │ L16 R16  │                 │ L16 R16  │
  └──────────┴──────────┴──────────┴─────────────────┴──────────┘
       ↑                     ↑                                  ↑
    播放头               写指针(我们)                     到头就绕回开头
   (PlayCursor)       (ByteToLock)
```

**大小从哪来**(L559):`SecondaryBufferSize = SamplesPerSecond * BytesPerSample * 2` = 48000 × 4 × 2。
那个 `* 2` 就是**2 秒**,而且它**只在 `WinMain` 出现这一处**(初始化函数只接受 `BufferSize` 参数,L120)——
一份真相。🟡 以后要调它,改这里一处即可。

**为什么是 2 秒**(§12.3,`01:12:45`):掉帧时宁可"**停更整块缓冲**"(声卡继续播旧数据,听感像还在播),
也不要"窄缓冲每秒循环 60 次"那种嗡嗡声。代价是**延迟**变大 —— 2D 游戏不敏感。

### 14.5 一个采样帧里到底算了什么(L475-489 / §五)

```c
internal void
Win32FillSquareWave(int16 *SampleOut, DWORD SampleCount)      /* L479-480 */
{
    for (DWORD SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
    {
        int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2)   /* L485 */
                            ? ToneVolume : -ToneVolume;
        *SampleOut++ = SampleValue;   /* 左声道 */
        *SampleOut++ = SampleValue;   /* 右声道 —— 同值 = 单声道听感 */
    }
}
```

- **`RunningSampleIndex` 有双重身份**:它既决定"写到环形缓冲的哪个字节"(14.6 第 2 步),
  又决定"当前方波处于哪个相位"。⇒ 所以它必须**全局**(L86)、**永远递增**、**跨帧不归零**(§四);
- **`HalfSquareWavePeriod`**(L90 全局,L557 赋值)= `(48000 / 256) / 2 = 93`:每 93 个采样翻转一次符号
  ⇒ 一个周期 186 个采样 ⇒ 实际音高 ≈ **258 Hz**(不是 256,整数除法,§五);
- **左右写同一个值** = 两个声道完全相关 = **听感居中(单声道)**(§8.3)。想有立体声差异就得给两声道
  不同的值(音量/延迟)—— 那是以后混音器的事,这一集不需要;
- **一帧只 `++` 一次**,因为循环体里写了**两个** `int16`。⚠️ 写成两次 ⇒ 相位两倍速 ⇒ 音高高八度。

### 14.6 每帧的七步(L678-737 / §六 §七 §八)

| 步 | 做什么 | 行 | 要点 |
|---|---|---|---|
| 1 | `GetCurrentPosition(&PlayCursor, &WriteCursor)` | L685 | 拿播放头**字节**位置;`WriteCursor` 我们**不用**(§7.2) |
| 2 | `ByteToLock = (RunningSampleIndex * BytesPerSample) % SecondaryBufferSize` | L692 | "我已经写了多少字节"折回环形 ⇒ 我该写哪 |
| 3 | 算 `BytesToWrite`(**三种情形**) | L698-713 | `>` 跨末尾:`(Size - ByteToLock) + PlayCursor`;`==` **第一帧**:填满整块;`<` 一段直写(§8.2) |
| 4 | `Lock(ByteToLock, BytesToWrite, &R1,&R1Size,&R2,&R2Size, 0)` | L717 | 跨末尾时会回**两块**(§六);`dwFlags = 0` |
| 5 | 写方波 | L724 / L727 | 两段**调同一个函数**(14.5)—— 公式只有一份 |
| 6 | `Unlock(R1,R1Size,R2,R2Size)` | L730 | ⚠️ **必须**,且参数用 Lock 给的原值(§十的咔哒声) |
| 7 | 首次 `Play(0,0,DSBPLAY_LOOPING)` | L733-737 | `if (!SoundIsPlaying)` 守卫 ⇒ **先填满再播**(§11.1) |

> [!question] 为什么第 3 步要分三种情形?
> 因为**写位置不能越过播放头**:越过就会改写"正在播"的数据。
> `ByteToLock` 在播放头**前面**时,得绕一圈分两段写;
> 而**刚启动**时两个游标都是 0,`PlayCursor - ByteToLock` 算出来是 0 ⇒ 什么都不写 ⇒
> 那就是"启动要等一秒"的根因,所以特判成"填满整块"(§11.2)。

### 14.7 两种帧:第一帧 vs 稳定帧(时序)

```
第一帧:  PlayCursor=0  ByteToLock=0        ⇒ 走 '==' 分支 ⇒ BytesToWrite=384000 ⇒ 填满整块
         └─▶ 然后 if(!SoundIsPlaying) ⇒ Play(...) ⇒ 播放头开始走

稳定帧:  播放头每秒走 192000 字节 ⇒ 60fps 下每帧约 3200 字节
         PlayCursor 前进 ≈3200,ByteToLock 也前进 ≈3200  ⇒ 只补这一小段
         └─▶ 缓冲里**始终留着约 2 秒的"余量"**(还没播到的数据)

掉一帧:  这一帧要补的字节变多(比如两帧的量)⇒ BytesToWrite 变大,但仍 ≤ 缓冲大小
掉很多帧:一旦追过整块缓冲 ⇒ 会覆写"正在播"的区域 ⇒ 听到杂音 / 爆音
         (这就是 2 秒余量在兜底:撑住 2 秒内的卡顿,超过就认输)
```

### 14.8 全局变量清单:为什么是这七个(L82-90)

| 变量 | 行 | 为什么必须全局 |
|---|---|---|
| `GlobalSecondaryBuffer` | L82 | 每帧要 `Lock`/`Unlock`,而初始化只发生一次 |
| `BytesPerSample` | L85 | 每帧算 `ByteToLock` 与两个 region 的采样数 |
| `RunningSampleIndex` | L86 | **跨帧递增**,归零就前功尽弃(14.5) |
| `ToneVolume` | L87 | 方波幅度(`±3000`,相对 32767 很轻 —— 现场"太吵"之后调的,`00:57:45`) |
| `SecondaryBufferSize` | L88 | 必须**等于**创建次缓冲时的 `dwBufferBytes`;不一致 ⇒ 绕环位置算错 |
| `SoundIsPlaying` | L89 | "只按一次播放键"的标志(§11.1) |
| `HalfSquareWavePeriod` | L90 | 每帧的写循环都要用它算相位;它属于"一套音频配置" |

> [!tip] 一条可复用的判据
> "这个变量要不要全局"在 Casey 那里等价于"**它后面还会不会被别人用**"(§四)。
> 不是"全局更方便",而是**生命周期由使用范围决定**。

### 14.9 我们这份实现的状态(2026-09-30)

**已经通了 ✅**

| 项 | 位置 |
|---|---|
| 初始化七步 + 五处失败诊断 | L119-215 |
| 环形缓冲 + 三种情形 + 两段写入 | L678-737 |
| `Play` + "先填满再播" | L733-737 |
| **重构**:两个 region 的循环合并成 `Win32FillSquareWave`(方波公式只剩一份) | L475-489 |
| **清理**:死变量 `SecondsToWrite` 删除;`SamplesPerSecond` 已参数化(消除"两份真相") | L120 / L553 / L560 |
| 编译:`/W4 /WX` 下 Debug / x64 与 Release / x64 全量 Rebuild **零警告** | 2026-09-30 实测 |

**还缺**(按优先级)

1. **计时 / 帧率控制** —— 现在是 busy loop,占满一个核;
2. **两段写入前的 `assert`** —— Casey 自己说 "this would be a good place for an assert";
   项目里还没有断言机制,等讲到那集再补;
3. **音效资源**:现在只有一个方波;没有 WAV 加载、没有混音器、没有音量 / 声像;
4. 🟡 **`HalfSquareWavePeriod` 的除零风险**:`Hz >= 48000` 时它变成 0 ⇒ L485 直接除零崩。
   现在 `Hz = 256` ⇒ 93,安全。

### 14.10 怎么证明它在工作(验证方法论)

| 想验什么 | 怎么做 | 本项目的实测结果 |
|---|---|---|
| 编得过、且没有隐藏问题 | `HandMade\build.bat`(或 VS F7) | `/W4 /WX` 零警告 = 免费的一轮检查 |
| 初始化每一步成没成 | `cdb -g -G x64\Debug\HandMade.exe`,stdout 里看那 5 句诊断 | 5 处字符串在 L178 / L182 / L203 / L208 / L213 |
| 真的是"运行期加载"而不是链接进来的 | `dumpbin /dependents` | 只有 `USER32/GDI32/KERNEL32` —— **没有** `dsound.dll` ⇒ 确认是 `LoadLibraryA` |
| 静态 CRT 有没有生效 | 同上 | 没有 `VCRUNTIME140*.dll` ⇒ `/MT` 生效 |
| 虚函数真的走 vtable | `dumpbin /disasm` 找 `call qword ptr [rax+30h]` | §2.3.4(第 6 格 = `SetCooperativeLevel`) |
| **声音对不对** | **用耳朵** | ⚠️ 我听不到 —— 这是你的活。听出"一秒空白"⇒ §11.1;听到"咔哒 / 嗡嗡"⇒ §十 或缓冲太窄 |

### 14.11 下一步会往哪长(预告,别现在做)

- **Day 009+**:混音器(把多个音源**相加**后写进同一块缓冲)、从 WAV 文件加载、音量与声像;
- **类银河城需要的音频**(不是 HH 地牢那一套):跳跃 / 落地 / 受伤 / 开门 / 环境音 ⇒
  **多音源同时响**是常态 ⇒ 混音器是必经之路;
- **架构上的终局**:游戏代码产生"**事件**"(比如"播放跳跃音")→ 混音器 → 同一块环形缓冲 ⇒ 平台层。
  也就是说:今天这套 `Lock / 写 / Unlock` 会一直在,只是**往里写的东西**从"一个方波"变成"混音结果"。

---

## 附录 A:本集新出现的 API / 常量速查(签名与常量都在本机 SDK 头文件核实过)

| 名字 | 作用 | 出处 / 实测值 |
|---|---|---|
| `IDirectSoundBuffer::GetCurrentPosition` | 拿 **play cursor** 和 **write cursor**(都是**字节**偏移) | `um\dsound.h` L706:`(LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor)`,两个参数都可选 |
| `IDirectSoundBuffer::Lock` | 锁住一段缓冲准备写;可能回**两块**区域 | L713:`(DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1, LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)` |
| `IDirectSoundBuffer::Unlock` | **写完必须调**,否则 DirectSound 会混乱(今天的咔哒声) | L723:`(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)` |
| `IDirectSoundBuffer::Play` | 开始播放 | L716:`(DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)`;**reserved1 必须 0** |
| `DSBPLAY_LOOPING` | 循环播放 | ✅ `0x00000001` |
| `DSBLOCK_FROMWRITECURSOR` / `DSBLOCK_ENTIREBUFFER` | Lock 的 flags(**我们都不用**) | ✅ `0x1` / `0x2` |
| `INT16_MAX` / `INT16_MIN` | 16 位有符号样本范围 | ✅ `32767` / `-32768` |
| `runningSampleIndex` | **我们自己**的"写了多少采样帧"计数器 | 无符号;`mod` 缓冲大小后得到环形位置 |
| `toneHz` / `halfSquareWavePeriod` | 音高 / 半周期采样数 | `halfPeriod = (SamplesPerSecond / ToneHz) / 2` |
| **vtable** | 编译器为每个"有虚函数的类型"生成的全局函数指针表 | 对象首部藏着指向它的指针;虚调用 = 两次间接寻址 |

---

## 附录 B:本集值得抄下来的几句话

1. **关于语言与 CPU**(`00:10:20`):
   > "everything gets translated to CPU… **languages make things sometimes simpler for the programmer,
   > they do not make things any simpler for the CPU**."
2. **关于全局变量**(`00:15:02` 起):
   > "**be aware of what you make global, don't do it willy-nilly**…
   > make sure you understand **why** it's global, that it **should stay** global, and that there's **only going to be one of them** —
   > and if any of those things aren't true, don't do it."
3. **关于环形缓冲的正确姿势**(`01:21:57`):
   > "I **always write it wrong the first time**… so what you want to do is **draw it out and verify** —
   > okay, we're actually writing to where we think we're writing."
4. **关于写法**(`00:50:48`):
   > "**compression-oriented programming** is the best way to do things… write the most obvious thing,
   > don't overthink it, and the patterns just kind of emerge."
5. **关于延迟**(`01:31:15`,他抱怨十年后给出的"答案"):
   > "the answer to *how much latency you want* is **nobody knows — it's completely unknown**."

---

## 附录 C:日程与其它(他亲口说的)

| 时间 | 内容 |
|---|---|
| 次日(周四) | **继续调试声音**;并**改用正弦波**(正式引入浮点) |
| 感恩节假期 | 周三/周四/周五 **上午 11:00** 直播 |
| 以后 | **每周五**都会是"欧洲友好"的时间 |
| 论坛 | 今天或明天上线(`handmadehero.org`) |

---

## 附录 D:本文档的事实核对来源

| 声称 | 怎么核实的 |
|---|---|
| **接口方法不是 DLL 的导出函数**(§2.4 图 9) | `dumpbin /exports C:\Windows\System32\dsound.dll` 实测:全部导出仅 12 个名字(`DirectSoundCreate` / `DirectSoundCreate8` / `DirectSoundCaptureCreate` / `DirectSoundCaptureCreate8` / `DirectSoundEnumerateA,W` / `DirectSoundCaptureEnumerateA,W` / `DirectSoundFullDuplexCreate` / `GetDeviceID` / `DllCanUnloadNow` / `DllGetClassObject`);`SetCooperativeLevel` / `CreateSoundBuffer` / `GetCurrentPosition` / `Lock` / `Unlock` / `Play` / `QueryInterface` **均未导出** |
| **`a->b` 等价于 `(*a).b`;`void *` 上不能用 `->`**(§2.4 图 8) | C++ 语言规则(编译器行为);并且 `dsound.h` L620 的 C 宏 `(p)->lpVtbl->SetCooperativeLevel(p,a,b)` 把这两步明写了出来 |
| `DirectSoundCreate` 是**平铺的导出函数**(不是方法)、`LPDIRECTSOUND` = `IDirectSound *`、`DSSCL_*` 的取值(§2.4 图 6) | 本机 `dsound.h` L471 / L480 / L166 / L2059–2062 |
| "先 `Create` 才能调 `SetCooperativeLevel`"是**硬约束**(§2.4 图 4) | 语言层面推导:方法调用需要一个对象;不需要外部文档 |
| "`SetCooperativeLevel` 必须在**创建任何缓冲之前**调" | 🟡 **只有视频口述**(我们的代码也照做了),我没找到可离线核对的文档原文 |
| `GetCurrentPosition` / `Lock` / `Unlock` / `Play` 的签名 | `C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\um\dsound.h` L706 / L713 / L716 / L723 |
| **vtable 的槽位顺序**(§2.3.3) | 同上头文件:`IDirectSound` L594–610、`IDirectSoundBuffer` L697–726;C 版本宏 `(p)->lpVtbl->Method(p,...)` 在 L616–624 |
| **槽位偏移 = 序号 × 8**(§2.3.4) | `dumpbin /disasm HandMade\build\win32_handmade.exe` 实测:`call qword ptr [rax+30h]` = `SetCooperativeLevel`(第 6 格)、`[rax+18h]` = `CreateSoundBuffer`(第 3 格)、`[rax+70h]` = `SetFormat`(第 14 格) |
| **vtable 指针在对象偏移 0、`this` 是第 1 个参数**(§2.3.4) | 同上 exe:`mov rax,[rax]`(先取表指针)、`mov rcx,[rsp+20h]`(再把对象当第 1 个参数传进去) |
| `DSBPLAY_LOOPING = 1`、`DSBLOCK_FROMWRITECURSOR = 1`、`DSBLOCK_ENTIREBUFFER = 2` | 同上头文件 |
| `INT16_MAX = 32767`、`INT16_MIN = -32768` | MSVC 的 `include\stdint.h` |
| "Lock 之后必须 Unlock" | 他当场念的 DirectSound 文档原文(`00:40:01`) |
| "`dwReserved1` 必须传 0" | 同上(`00:56:43`) |
| 时间戳引用 | `explain\!subtitle\[Mira] Handmade Hero Day 008 - Writing a Square Wave to DirectSound.txt`(⚠️ ASR 文本,数字/名字可能有误) |
| 本集他没做的 | 没加载 WAV、没做混音器、没引入浮点(明天)、**环形缓冲代码他自己认定还有 bug** |


---

## 附录 E:音频系统的延伸学习(同风格资源)

> 记录于 2026-09-30。集数与标题取自官方 [Episode Guide](https://guide.handmadehero.org/)(逐条核过);
> 我看不了视频本身,所以**不评价"哪一集讲得好"**,只报主题与出处。

### E.1 同一部教程里的音频主线(直接往下看)

| 集 | 标题 | 主题 |
|---|---|---|
| [009](https://guide.handmadehero.org/code/day009/) | Variable-Pitch Sine Wave Output | **方波 → 正弦波**,顺带把浮点 / 定点讲透(下一集就是它) |
| [010](https://guide.handmadehero.org/code/day010/) | QueryPerformanceCounter and RDTSC | 计时 —— 音频同步的前提 |
| [012](https://guide.handmadehero.org/code/day012/) | Platform-independent Sound Output | ★ **架构转折**:把"产生样本"移到游戏层,平台层只负责送 |
| [019](https://guide.handmadehero.org/code/day019/) | Improving Audio Synchronization | 音画同步 |
| [020](https://guide.handmadehero.org/code/day020/) | Debugging the Audio Sync | 上面那件事的调试 |
| [138](https://guide.handmadehero.org/code/day138/) | Loading WAV Files | 加载 WAV |
| [139](https://guide.handmadehero.org/code/day139/) | Introduction to Sound Mixing | ★ 混音器(把多个音源相加)—— 正是 14.11 预告的那一步 |
| [140](https://guide.handmadehero.org/code/day140/) | Implemented a Sound Mixer | 混音器落地 |
| [141](https://guide.handmadehero.org/code/day141/) | Streaming Large Audio in Chunks | 大音频流式播放 |
| [142](https://guide.handmadehero.org/code/day142/) | Per-sample Volume Interpolation | 音量逐样本插值(消爆音) |
| [143](https://guide.handmadehero.org/code/day143/) | Pitch Shifting in the Mixer | 变调 |
| [144](https://guide.handmadehero.org/code/day144/) | SSE Mixer Pre and Post Loops | 混音器 SIMD 优化 |
| [145](https://guide.handmadehero.org/code/day145/) | SSE Mixer Main Loop | 同上(主循环) |
| [146](https://guide.handmadehero.org/code/day146/) | Accumulation vs. Explicit Calculation | 累加 vs 显式计算 |
| [524](https://guide.handmadehero.org/code/day524/) / [526](https://guide.handmadehero.org/code/day526/) | Integrating WAV Importing / Single-Buffer Sound Streaming | 后期:WAV 进资产管线 + 单缓冲流式 |

### E.2 其它资源(非 HH)

| 资源 | 是什么 | 为什么值得看 | 注意 |
|---|---|---|---|
| Guy Somberg《Game Audio Programming: Principles and Practices》([CppCon 2017 演讲 PDF](https://github.com/ghalpern25/CppCon2017/blob/c63ff388a88008095181c08443bc45543a9cee68/Presentations/Game%20Audio%20Programming%20in%20C%2B%2B/Game%20Audio%20Programming%20in%20C%2B%2B%20-%20Guy%20Somberg%20-%20CppCon%202017.pdf)) | 专讲**游戏音频引擎** | 混音器 / 流式 / voice 管理怎么组织 —— 最接近"引擎层"的视角 | 例子是 C++ / 面向对象,与 `ADR 0005` 的 C 风格不同:**看结构,别抄风格** |
| [miniaudio](https://github.com/mackron/miniaudio)(单文件 C 库) | 一个人写的完整音频库 | **读源码**:设备后端(WASAPI / DirectSound / ALSA)+ 混音 + 环形缓冲,全在一个文件里 | 它是**库**;引入库违反 `ADR 0005` ⇒ 只当读物 |
| [Ross Bencina — Real-time audio programming 101](https://lwn.net/Articles/452630/) | 实时音频编程的经典忠告 | "音频回调里不要加锁、不要分配、不要做 I/O" —— 做混音器时的救命文档 | 偏理论,没有 Win32 代码 |
| 《The Scientist and Engineer's Guide to Digital Signal Processing》(Steven W. Smith) | 免费在线的 DSP 入门书 | 采样 / 混叠 / 滤波 / 重采样 —— 后面绕不过去 | 🟡 只核了书名与"免费在线",没核具体章节 |
| WASAPI / XAudio2 官方文档 | Windows 现代音频 API | Casey 在 Day 008 的 Q&A 里自己说"XAudio 大概是现在大家用的" | DirectSound 是**遗留**(§2.3.4 那个"只导出 12 个名字"就是证据);换 API 是以后的大决定,别现在动 |

🟡 我不评价这些资源的**质量高低**(看不了视频),只列主题与出处。

---

## 附录 F:从零讲透 —— 音频系统到底在干什么(小白版)

> 这一节**不假设你懂任何音频知识**,也不看行号、不查 API —— 只讲"为什么是这样"。
> 想对照我们代码的真实行号,回 §十四。看不懂 §十四 的时候,回来读这一节。

### F.1 计算机是怎么"存声音"的?

声音的本质是**空气在振动**:喇叭的膜一前一后地动,把空气压出波纹,耳朵就听到了。

计算机不会存"振动",它只会存**数字**。于是有个笨办法:

> **每秒量很多次膜的位置,每次记一个数。**

- **每秒量多少次** = **采样率**(我们的 `SamplesPerSecond = 48000`,即每秒 48000 次);
- **每次记的那个数** = **一个采样**(sample)。我们用 16 位有符号整数:`-32768 ~ +32767`,
  其中 **0 = 膜在正中间**(不动);
- 想让喇叭往前推 → 给正数;往后拉 → 给负数。**数字在 0 上下摆动,膜就在中间来回摆,那就是声音。**

所以:**"播放声音"不是"把一个文件交给声卡",而是"按固定节奏、每秒 48000 次,把数字喂给它"。**

### F.2 六个概念(定义 + 为什么 + 在我们的代码里叫什么)

| 概念 | 一句话定义 | 为什么需要它 | 我们的代码里 |
|---|---|---|---|
| **采样 sample** | 某时刻膜的位置,一个 16 位数 | 数字化的最小单位 | 循环里那个 `SampleValue` |
| **采样率 sample rate** | 每秒量多少次 | 播放端必须**用同样的速度**读,否则音高就变了 | `SamplesPerSecond = 48000` |
| **声道 channel** | 有几个喇叭(我们:左 + 右) | 每个时刻要给**每个喇叭**各一个数 | `nChannels = 2` |
| **采样帧 frame** | **同一时刻的全部声道**的采样 = 左 + 右 | 时间上"一个点"的完整描述 | 4 字节(`BytesPerSample`) |
| **PCM 格式** | "这串数字怎么解释"的说明书 | 系统得知道按什么速度、几位、几声道去读 | `WAVEFORMATEX` |
| **缓冲 buffer** | 一块内存,声卡**自己**反复读它 | 我们不能"一次喂一个数",只能提前准备一批 | `GlobalSecondaryBuffer` |

> [!warning] 新手最大的坑:三个"数量单位"不是一回事
> - **样本(sample)** = 一个 16 位数 = 2 字节;
> - **帧(frame)** = 一个时刻的全部声道 = 左 + 右 = **4 字节**(`BytesPerSample`);
> - **字节(byte)** = 系统真正用来定位的位置单位(`PlayCursor`、`ByteToLock` 都是字节)。
>
> 同一段内存,用三个单位量出来是**三个不同的数**。我们代码里之所以到处除以 4,
> 就是因为"我要写 N 帧,但 API 问我要多少**字节**"。
> 🟡 这一条几乎每个人都会栽一次 —— 包括"左右声道写成同一个值是不是错了"这类困惑(答案:没错,见 §8.3)。

### F.3 关键直觉:声卡是一台**不会停**的机器

这是整个音频系统里最反直觉、也最重要的一点:

> **声卡按自己的节奏往前走,谁也停不了它。**
> 它每秒固定吃掉 `48000 × 4 = 192000` 字节(`nAvgBytesPerSec`),吃完就回头再吃一遍。

于是问题变成:

- 我们**每秒只有 60 次机会**(每帧一次)往那块内存里写东西;
- 每次大约写 `192000 ÷ 60 ≈ 3200` 字节;
- **写慢了** → 声卡读到的是上一次的旧数据 ⇒ 卡顿/断音;
- **写快了/写多了** → 覆盖了它**还没播到**的数据 ⇒ 爆音/杂音。

**这两个约束一夹,唯一可行的结构就是环形缓冲 + 追着播放头写**:

```
   声卡在跑 ──▶ 播放头(PlayCursor)一直往前
   我们在追 ──▶ 写指针(ByteToLock)必须追在它后面,但不能被它追上
                        写指针                播放头
   ┌───────┬───────────┼─────────────────────┼───────────────┐
   │ 旧数据 │ 不能覆盖   │   ★ 我们写的这一段   │  还没播到的    │
   └───────┴───────────┴─────────────────────┴───────────────┘
                          ← 这段就是"余量" →
```

- **"环形"**:写到最后就绕回开头继续写,所以永远不会"用完";
- **"2 秒"**:缓冲有 384000 字节 = 2 秒的量。它的意思是"**我可以落后最多 2 秒**" ——
  掉帧、卡一下都有救,超过 2 秒才真的断。
- 为什么不用 1 毫秒的小缓冲?那样延迟极低(适合演奏),但**每一次卡顿都会立刻听到**;
  2D 游戏不在乎那点延迟,所以选大的(§12.3 里他讲了两种方案的取舍)。

### F.4 初始化:用"搬工具箱"来理解那七步

| 步骤 | 类比 | 代码在做什么 |
|---|---|---|
| `LoadLibraryA("dsound.dll")` | 把别人的**工具箱**搬进屋子 | 把 DLL 载入进程,里面的代码/数据才能用 |
| `GetProcAddress("DirectSoundCreate")` | 从工具箱里**按名字找出**那把工具 | 拿到函数地址(它不在导入表里,所以必须运行期查,见 `ADR 0004`) |
| `DirectSoundCreate(0,&DS,0)` | 让系统**造一台音频设备对象**给我 | 得到一个带函数表(vtable)的对象 |
| `SetCooperativeLevel(窗口, PRIORITY)` | 跟系统**签约**:我要按高优先级用设备,并且要求"能改主缓冲格式" | 没有这一步,后面的 `SetFormat` 会被拒绝 |
| 主缓冲 + `SetFormat` | 告诉声卡:**以后按 48000 / 16 位 / 2 声道播** | 主缓冲不是一块数据,它只是"改格式的把手" |
| 次缓冲(384000 字节) | 领到**我们要写的那块内存** | 输出给全局 `GlobalSecondaryBuffer` |
| 五处失败诊断 | 每一步失败都**留个字条,但继续跑** | 没有声音也照样能玩游戏 |

> 📌 为什么每一步失败都只"降级"不"崩溃"?因为**音频是可选的**:缺声卡、缺 DLL 都不该让游戏打不开。

### F.5 每帧:用人话把七步走一遍

把声卡想成一台**不停转的唱机**,我们要往唱片上刻音符,而**唱针已经转过去了的地方不能再刻**。

| 步 | 人话 | 代码 |
|---|---|---|
| 1 | 问唱机:"你唱到哪了?" | `GetCurrentPosition` → `PlayCursor` |
| 2 | 算"我该从哪刻起" | `ByteToLock = (RunningSampleIndex × 4) % 384000` |
| 3 | 算"这次能刻多少" —— **刻到唱针前面为止**;若会跨过唱片末尾,就分成两段 | 三种情形的 `if` |
| 4 | 请系统把那一段"借"给我(它顺便告诉我分成了哪两块) | `Lock` → `Region1` / `Region2` |
| 5 | **刻音符**:写方波数字(左 = 右 = 同一个值) | `Win32FillSquareWave` ×2 |
| 6 | 还回去 —— **不还就会咔哒响** | `Unlock` |
| 7 | 第一帧结束时再按下"开始转" | `if (!SoundIsPlaying) Play(...)` |

**注意第 3 步的两种特殊情况**:

- **刚开始时**,唱针在 0、我们也在 0 ⇒ 算出来"能刻 0 个" ⇒ 那就**把整张唱片刻满**(这就是"先填满再播");
- **每帧正常**时,我们只补"唱针刚过去的那一小段",所以缓冲里**永远留着大约 2 秒还没播的余量**。

### F.6 方波到底是个什么东西?

我们这一集写的"音"是**方波**:它的值只有两种 —— **+3000 和 −3000**(`ToneVolume`),在两者之间来回跳:

```
   +3000 ┌────┐    ┌────┐    ┌────┐
         │    │    │    │    │    │
     0 ──┼────┼────┼────┼────┼────┼──▶ 时间
         │    │    │    │    │    │
   -3000 └────┘    └────┘    └────┘
          ←93→←93→   一个周期 186 个采样
```

- **多久翻一次?** 每 `HalfSquareWavePeriod = 93` 个采样翻一次 ⇒ 一个周期 186 个采样;
- **听起来是几赫兹?** `48000 ÷ 186 ≈ 258 Hz`(我们想写 256,但整数除法让它偏了一点点,见 §五);
- **为什么先用方波?** 因为它**最好算**:只要"翻符号",不用 `sin`、不用浮点(浮点是下一集 Day 009 才讲的);
- **为什么听起来刺耳?** 方波在数学上等于"很多个纯正弦叠起来",里面含大量高频成分 ——
  以后讲到正弦波/傅里叶就明白了(而且这也是为什么他下一集要换成正弦波来调试)。

### F.7 四个新手常见的误解(我猜你可能也这么想过)

| 误解 | 实际 |
|---|---|
| "播放声音 = 调个 API 把文件播出去" | ❌ 是**持续供应数字流**:我们每帧往一块内存里填数,声卡自己按节奏读 |
| "缓冲区大小 = 一次能播多久" | 半对:它是"**我能提前准备多少**"的余量,不是"这首歌多长" |
| "48000、16 位、2 声道是随便定的" | ❌ 由设备和格式约定决定;改一个数字,整套换算全变(所以它们被抽成了变量) |
| "字节数 = 样本数 = 帧数" | ❌ 三个单位,差 2 倍或 4 倍 —— 这是本主题**头号** bug 来源 |

### F.8 出问题时:现象 → 原因对照表(排错用)

| 你听到/看到 | 最可能的原因 | 去哪查 |
|---|---|---|
| **完全没声音** | 没调 `Play`;或初始化某步失败 | 用 `cdb -g -G` 看那 5 句诊断(§14.10) |
| **启动要等一秒才响** | 先播后填,不是"先填满再播" | §11.1 |
| **周期性"咔哒/嗡嗡"** | 忘了 `Unlock`;或者缓冲太小(每秒循环几十次) | §十、§12.3 |
| **音高高了八度** | `RunningSampleIndex` 一帧加了两遍 | §14.5 |
| **声音又慢又低** | 声明的采样率与实际写入量不一致("两份真相") | §14.4 |
| **播一会儿就停** | 掉帧时间超过了缓冲余量(2 秒) | §14.7 |
| **画面在动但声音不同步** | 音画同步本来就不完美(页翻转时机不可测) | §12.3 |

### F.9 术语小词典(这一主题专用)

| 词 | 意思 |
|---|---|
| **sample 采样** | 一个 16 位数,描述某一时刻膜的位置 |
| **sample rate 采样率** | 每秒多少个采样(48000) |
| **channel 声道** | 几个喇叭(2 = 左 + 右) |
| **frame 帧** | 同一时刻所有声道的采样(= 4 字节)。⚠️ 与"渲染的帧"不是一个东西 |
| **interleaved 交错** | L,R,L,R… 这样排;不是"先全部左,再全部右" |
| **PCM** | 最朴素的编码方式:直接存采样值(我们用的就是它) |
| **buffer 缓冲** | 一块内存;声卡在那上面循环读 |
| **ring buffer 环形缓冲** | 写到末尾绕回开头,所以"永远写不完" |
| **cursor 游标** | 位置指示:播放头(`PlayCursor`)、写指针(`ByteToLock`) |
| **主缓冲 primary** | 不是数据区,是"改设备格式的把手" |
| **次缓冲 secondary** | 真正被我们写、被声卡读的那块内存 |
| **HRESULT** | Windows 的"返回值 + 错误码"约定:`SUCCEEDED(hr)` 判成功 |

### F.10 接下来你会遇到的概念(先混个脸熟)

1. **正弦波与浮点**(Day 009):`sinf` 返回 −1..1 的小数 ⇒ 必须讲浮点/定点;
2. **平台无关的声音输出**(Day 012):"产生样本"这件事要从平台层搬到**游戏层** ——
   平台层只负责"把给好的样本送出去"。这是架构上的一小步,思想上一大步;
3. **混音器**(Day 139–146):多个声音同时响时要**相加**;以及为什么必须**限幅**(相加会溢出);
4. **延迟与缓冲大小的取舍**:小缓冲 = 低延迟但易爆音;大缓冲 = 稳但有延迟。
5. **采样率转换(重采样)**:素材是 44100、设备是 48000 时怎么办。

> 上面这些集数来自官方 [Episode Guide](https://guide.handmadehero.org/)(已核,详见附录 E)。