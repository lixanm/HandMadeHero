# Day 004 —— Animating the Backbuffer(让后备缓冲动起来)

> **素材**:`C:\Users\thoth\Downloads\[Mira] Handmade Hero Day 004 - Animating the Backbuffer.txt`
> (3299 行字幕,约 1 小时;本文件按内容重排)
> **本文件**:`D:\HandMade\explain\.HandmadeHeroExplain\Day 004 - Animating the Backbuffer.md`
> **一句话总结**:**我们自己写的像素第一次动起来了** —— 从此"渲染"完全归我们,Windows 只负责显示

---

## 一、这一集干了什么(总览)

| 阶段 | 内容 |
|---|---|
| 0:00 ~ 4:15 | **开场更正**(很重要):不需要 DC、也不需要位图句柄了 |
| 4:15 ~ 17:00 | 改成**自己分配内存**:`VirtualAlloc` / `VirtualFree` / 页与提交 |
| 17:00 ~ 25:00 | 把 blit 改成**整窗刷新**;搞清 **pitch/stride** 与 **自上而下 vs 自下而上** |
| 25:00 ~ 40:00 | **正式画像素**:逐通道写 → 发现 BGR 之谜 → 做出渐变 |
| 40:00 ~ 58:00 | 抽出 `RenderWeirdGradient`、**改成 `PeekMessage` 游戏循环**、让画面动起来;改成**一次写 32 位** |
| 58:00 ~ 结束 | Q&A |

**成果**:一个自己渲染、自己 blit、正在动的彩色渐变窗口。他说"我们用不到 3 小时把机器回到了 1980 年代"——意思是**我们又拥有了一块可以随便画的位图**,接下来想画什么就画什么。

---

## 二、开场的关键更正:DC 和位图句柄都不要了

有人(Chris Hecker,当年写过 WinG —— Windows 3.1 上第一套能快速 blit 的库)提醒他:昨天纠结"要不要 `CreateCompatibleDC`"其实是想岔了。

**真相**:

| 函数 | 接收什么 | 需不需要 DC / 位图句柄 |
|---|---|---|
| **`BitBlt`** | **DC → DC**(不能直接给指针) | ✅ 需要:你得先把位图选进一个 DC,再从那个 DC blit 到窗口 DC |
| **`StretchDIBits`** | **直接给像素指针** | ❌ **完全不需要** |

他昨天之所以心心念念"必须让 Windows 分配内存",是因为**他平时一直用 `BitBlt`**(而 BitBlt 确实需要那一套)。当年 `StretchDIBits` 是"慢路径",但那是在没有 3D 显卡加速的年代;现在这个差别基本不存在了,而且我们以后优化时会走完全不同的路(直接写 GPU 纹理),所以:

> **用 `StretchDIBits` + 自己分配的内存就够,`HBITMAP` 和 DC 全都可以删掉。**

**所以要删掉的东西**(📍我们代码里对应的部分):

- ❌ `BitmapHandle`(位图句柄)
- ❌ `BitmapDeviceContext`(那个"格式参照" DC)
- ❌ `CreateDIBSection` / `CreateCompatibleDC` / `DeleteObject`
- ✅ 保留:`BitmapInfo`(只要把头填对:宽、高、位深)、`BitmapMemory`(改成自己 `VirtualAlloc`)

**为什么还是 32 位而不是 24 位**:x86 上**未对齐访问有性能惩罚**——4 字节的值最好落在 4 的倍数地址上。24 位 = 3 字节,永远对不齐(得 12 字节才回到边界);所以要 32 位,每像素正好 4 字节。

```c
int BytesPerPixel = 4;
int BitmapMemorySize = Width * Height * BytesPerPixel;   // 宽 × 高 × 每像素字节数
```

---

## 三、自己分配内存:`VirtualAlloc`

```c
BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
```

| 参数 | 填什么 | 说明 |
|---|---|---|
| 起始地址 | `0` | 我们不在乎内存放哪。**传 0 = 让系统挑**,而且地址传 0 时大小会被**向上取整到页边界** |
| 大小 | `BitmapMemorySize` | 字节数 |
| 分配类型 | `MEM_COMMIT` | 见下 |
| 保护 | `PAGE_READWRITE` | 可读可写;**不需要执行权限**(我们不在里面跑代码) |

### 页(page)与 reserve / commit

- 内存是按**页**分配的,Windows 上一页通常是 **4096 字节**。
- **reserve(保留)**:只是把地址空间"占个位",让系统知道这块归你了,但**还不真的给你内存**。
- **commit(提交)**:真的开始跟踪这块内存、接到物理内存上。`MEM_COMMIT` 就是这个。
- **decommit**:停止跟踪,但**保留地址范围**(`MEM_DECOMMIT`)。

**为什么用 `VirtualAlloc` 而不是 `malloc`**:`malloc` → CRT → `HeapAlloc` → 最终**也是** `VirtualAlloc`。他明说:这个项目我们**几乎全部自己做内存管理**,所以现在就养成直接调 `VirtualAlloc` 的习惯。

### 释放与"抓到悬空指针"的技巧

```c
VirtualFree(BitmapMemory, 0, MEM_RELEASE);   // 彻底还给系统,大小传 0 也行(它记得)
```

- `MEM_RELEASE` = 整块释放,还给系统。
- 另有一个很实用的调试技巧:**先 `VirtualProtect(mem, size, PAGE_NOACCESS)` 把页面设成完全不可访问,而不是直接释放**。这样如果还有代码留着旧指针去写它,会**当场炸在你自己的那一行**上 —— 否则那块内存可能已被系统分给别人,你会写出一个极难查的"use-after-free" bug。他说以后会做这件事。

---

## 四、pitch / stride,以及"上"到底是哪一边

### 1. 一维内存怎么装二维图像

内存就是一长串字节;一个二维图像按"一行接一行"放进去。问题是:**一行读完了,下一行在哪?**

| 术语 | 定义(他的说法) |
|---|---|
| **pitch** | 从"这一行开头"移到"下一行开头"要加的字节数 |
| **stride** | 从"这一行结尾"移到"下一行结尾"要加的字节数 |

**为什么不能想当然**:因为有些图形格式**每行末尾有填充字节**(为了对齐),这时 pitch ≠ 宽 × 每像素字节数。虽然我们现在 32bpp 正好没有填充,但他建议**按"有 pitch 这回事"的方式来写循环**,以后不容易错。

### 2. `biHeight` 的正负决定"第一行在内存里是画面的哪一行"

他现场查了文档:

| `biHeight` | 位图方向 | 原点 |
|---|---|---|
| **正数** | **自下而上(bottom-up)** | **左下角** —— 内存第一行其实是画面**最下面**那行 |
| **负数** | **自上而下(top-down)** | **左上角** —— 内存第一行就是画面**最上面**那行 |

他**选了负数**(自上而下),理由:

- Windows 的坐标系本来就是"Y 往下增长",图形编程里大家也习惯这样;
- 老式帧缓冲也是从顶部开始往下。

📍**这条直接关系到我们**:我们现在 `biHeight = Height`(正数),所以内存里第一行是画面的**最下面**一行。纯色看不出来,画渐变(颜色跟 Y 相关)就会上下颠倒 —— 要么按他改成负数,要么写循环时从最后一行开始。

---

## 五、开始画像素:第一个像素循环

### 1. 先定义几个好打字的类型名

```c
#include <stdint.h>     // 或者自己 typedef(Casey 自己定义)

typedef uint8_t  uint8;    // 无符号 8 位  = 0~255,正好一个颜色通道
typedef uint16_t uint16;
typedef uint32_t uint32;
```
他觉得 `unsigned char` 又长又没表达意思 —— 他要的是"**无符号 8 位整数**"。

### 2. 循环的写法

```c
uint8 *Row = (uint8 *)BitmapMemory;          // ★ 先转成"按字节"的指针
for (int Y = 0; Y < BitmapHeight; ++Y)
{
    uint32 *Pixel = (uint32 *)Row;           // 每一行:转成"按像素"的指针
    for (int X = 0; X < BitmapWidth; ++X)
    {
        *Pixel++ = ... ;                     // 写一个像素,指针前进一个像素
    }
    Row += Pitch;                            // ★ 行指针按"字节"前进一整行
}
```

**为什么行指针要用 `uint8 *`**:C 里对指针做加减时,会**偷偷乘上"所指类型的大小"**。用 `uint8 *`(每个 1 字节)才能**显式地按字节移动**,不受类型大小干扰。

### 3. 第一次写错了:明明是"红",屏幕上却是蓝

他故意做了个"隔离实验"——先把其他通道都写 0,往他**以为**是红的那个字节写 255:

```c
Pixel[0] = 0;     // 他以为是绿
Pixel[1] = 0;     // 他以为是蓝
Pixel[2] = 0;     // 他以为是 pad
Pixel[3] = 0;
// 往"他以为是红"的那个字节写 255
```

**结果屏幕是蓝的。** 原因(他讲得很透):

1. x86 是**小端(little-endian)**:把一个 32 位值从内存读进寄存器时,**内存的第一个字节会变成最低 8 位**;
2. 于是"内存里按 R,G,B,pad 排"在**寄存器里看**就成了 **B,G,R,pad** —— 反的;
3. **Windows 的作者不干**:他们希望**在寄存器里看**是 R,G,B 的顺序,所以干脆把**内存顺序定义成反的**:
   > **所有 Windows 位图的内存顺序都是:蓝、绿、红、pad。**

4. 而 pad 为什么还留在最高位?因为**他们只在乎 R/G/B 三个的相对顺序,没管 pad** —— 所以只把红和蓝换了个位置,pad 就留在原地了。

**pad 那个字节有用吗?** 有:**如果你用的是"分层窗口(layered window)",它可以当 alpha(透明度)通道用**。我们的游戏不需要背景透出来,所以没用它。

---

## 六、做出渐变 + 让它动起来

### 1. 先做个静态渐变

```c
// 蓝通道跟着 X 变,绿通道跟着 Y 变,红留空
Pixel[0] = (uint8)(X + XOffset);   // 蓝
Pixel[1] = (uint8)(Y + YOffset);   // 绿
Pixel[2] = 0;                       // 红
Pixel[3] = 0;                       // pad
```

效果:因为只用了 8 位,所以**每 256 个像素颜色就"回绕"一次** —— 屏幕上一道一道的彩条。

### 2. 抽成函数,好让它动

```c
internal void RenderWeirdGradient(int XOffset, int YOffset)
```
把"往每像素写什么颜色"抽出来,接受两个偏移量。它们加在颜色里,**看起来就像整片渐变在滑动** —— 注意:**他偏移的不是坐标,是颜色值**(他后来说其实应该叫 `BlueOffset` / `GreenOffset` 更准确)。

### 3. 从 `GetMessage` 换成 `PeekMessage`(关键改动)

**为什么必须换**:`GetMessage` 在队列空时会**阻塞**,程序就停在那里等消息,无法每帧重画。游戏必须**一直跑**。

```c
while (Running)
{
    MSG Message;
    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))   // ★ 内层循环:把队列排空
    {
        if (Message.message == WM_QUIT)   // 不再靠 GetMessage 返回 0 了,要自己看
        {
            Running = false;
        }
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }

    RenderWeirdGradient(XOffset, YOffset);

    // ★ 渲染完必须自己 blit 到窗口,否则屏幕上什么都不会变
    HDC DeviceContext = GetDC(Window);
    StretchDIBits(DeviceContext, 0, 0, Width, Height, 0, 0, BitmapWidth, BitmapHeight,
                  BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(Window, DeviceContext);

    ++XOffset;      // ★ 每帧把偏移 +1 → 动起来
}
```

| 要点 | 说明 |
|---|---|
| `PeekMessage` | 和 GetMessage 参数一样,但**多一个标志**。没有消息时**立刻返回**,不阻塞 |
| `PM_REMOVE` | "把消息**取走**"(而不是只看一眼留着)。返回非 0 = 有消息 |
| **内层 while** | 一帧里要把队列**排空**再渲染。系统要求你必须处理队列,否则程序行为会变得很怪 |
| 不用 `GetMessage` 后 | 就**拿不到"返回 0 = WM_QUIT"这个信号**了,所以要在循环里**自己检查 `WM_QUIT`** 并把 `Running = false` |
| 循环里 blit | 他在 WM_PAINT 之外**每帧主动 blit**;`GetDC`/`ReleaseDC` **成对出现**(他还专门说:这种"取-还成对"的地方,用 RAII/构造析构封装他完全没意见) |

### 4. 他现场犯的两个错(很有教学价值)

| 错误 | 现象 | 原因 |
|---|---|---|
| **只在内存里渲染,忘了 blit** | 屏幕毫无变化 | 渲染的是内存里的位图,不 blit 就永远到不了屏幕 |
| **`XOffset` 声明在循环体里面** | 画面不动 | 每帧都被重新初始化为 0。**要放到循环外面**才能累积 |

### 5. 最后:改成"一次写 32 位"

写 4 个字节 → 改成**拼成一个 32 位值、一次写进去**:

```c
uint8 Blue  = (uint8)(X + XOffset);
uint8 Green = (uint8)(Y + YOffset);
*Pixel++ = ((Green << 8) | Blue);
```

为什么这样能对上?因为内存顺序是 **B,G,R,pad**,而小端机器上"32 位值的最低 8 位"正好落在内存的第一个字节:

```
寄存器里的 32 位值:  [pad][红][绿][蓝]      ← 高 8 位 ... 低 8 位
内存里的顺序:         [蓝][绿][红][pad]      ← 低地址 ... 高地址
                        ↑ 所以蓝放最低 8 位、绿位移 8 位后"或"上去
```

他顺带把 `uint32` 的 typedef 补上了,还说他打算以后把这些类型**搬到一个平台无关的头文件**(就是后来 Handmade Hero 里的 `handmade.h`)。

---

## 七、Q&A 要点

| 问题 | 他的回答 |
|---|---|
| 为什么用 `0` 而不是 `NULL` | 他喜欢**少一点 #define**。现代平台上 `NULL` 就是 0,直接用 0 更省事 |
| 为什么用 `stdint.h` 的类型却还自己 typedef | 老年代**必须**自己定义(不同平台 int 可能是 16 位);现在 `stdint.h` 已经有了,新手直接用标准类型也行。他还专门向观众**求证 `static` 是不是"零初始化"的唯一来源** —— 结论:**是的,`static` 才是保证零初始化的那个**,只是"全局"并不保证(他不信 Stack Overflow,要人去查标准) |
| `malloc`/`new` 和 `HeapAlloc` 有什么区别 | `HeapAlloc` 是系统的堆分配器;`malloc`/`new` 是 CRT 的包装,**最终也是调 `HeapAlloc` → `VirtualAlloc`**。他说自己不用它们,所以不便多说 |
| 把位图分配在**栈**上会不会更快 | 不会。而且**根本不行**:栈上的东西**函数一返回就没了**。需要"活过这个函数"的东西才需要分配。`VirtualAlloc` 必须给整页(4096 字节),少于这个也是给一整页 |
| `PAGE_WRITECOPY` 是干什么的 | 写时复制(copy-on-write):先共享同一份物理内存,**一旦有人写,就偷偷复制一份新的给他**。他说这多半是为了实现 Unix 的 `fork()` 才进 Windows 的 |
| pitch 和 stride 到底怎么算 | (他重新讲了一遍,内容见第四节) |
| 为什么内存顺序是 B,G,R,pad 而不是 pad 在最前 | 见第五节 4:Windows 作者**只想让 R/G/B 的顺序对**,没管 pad |
| pad 能当 alpha 用吗 | 能,如果你用**分层窗口**的话 |
| **窗口上为什么是"忙碌"光标?(windows wait symbol)** | **因为我们还没设置光标**。他现场把光标去掉了 —— 这对游戏正好(游戏要自己画光标)。📍**我们也一样**:`hCursor` 被注释掉了,所以窗口上是沙漏光标 |
| 内存泄漏 | 用任务管理器看:占用约 10 MB,**反复 resize 保持稳定**(释放和重新分配 `VirtualAlloc` 是正常的) |
| 性能能测吗 | **测不了**:OBS 用桌面捕获把机器拖到接近 0 帧,鼠标一动更慢。他急需一张**采集卡**才能做性能分析 |
| 明天(周五)干什么 | **不讲新东西**,把这一周的代码**慢慢过一遍、清理一下**,深入讲每个细节 |

---

## 八、和我们代码的关系 / 下一步(重要)

Day 4 对我们现在这份代码有几处**直接冲突的更正**:

| Day 4 说的 | 我们现在(`win32_handmade.cpp`) | 要不要改 |
|---|---|---|
| **不要再让 Windows 分配位图**:删掉 `CreateDIBSection`、`HBITMAP`、那个兼容 DC | 我们昨天(按 Day 3 文档)正是用 `CreateDIBSection` + `BitmapHandle` + `BitmapDeviceContext` | ✅ **要改**:改成 `VirtualAlloc` 自己分配,删掉这三个东西 |
| `biHeight` 用**负数** = 自上而下 | 我们填的是正数(自下而上) | ✅ **要改**(不然画渐变会上下颠倒) |
| blit 时**整窗刷新**(目标 `0,0,W,H`),先别管脏区域 | 我们用 `rcPaint` 的脏区域 | ✅ 改成整窗,先简化调试 |
| 循环里**每帧渲染 + blit**,用 `PeekMessage` | 我们还在 `while (Running) { GetMessage(...) }` 阻塞等消息 | ✅ **要改**:换成 `PeekMessage` + 内层排空 + 每帧渲染/blit |
| 抽出 `RenderWeirdGradient(X, Y)` | 我们那段"临时纯色"填在 `ResizeDIBSection` 里 | ✅ 抽成函数,删掉临时纯色 |
| 一次写 32 位像素(`(Green << 8) \| Blue`) | 我们临时那段是逐像素写 `0x00996633` | ✅ 改成他的渐变写法 |
| `uint8` / `uint32` typedef | 我们还没定义 | ⬜ 加上(`stdint.h` 或自己 typedef) |
| 加 `-FC` 编译参数(报错带完整路径,Emacs 里能直接跳行) | 📍**我们的 `build.bat` 里已经有 `-FC` 了** | ✅ 已完成 |

### 我们下一步要做的(Day 4 版),按顺序

1. **删**:`BitmapHandle`、`BitmapDeviceContext`、`CreateDIBSection`、`CreateCompatibleDC`、`DeleteObject`
2. **改**:`ResizeDIBSection` 里改成 `VirtualAlloc`,并加 `VirtualFree` 释放旧内存;`biHeight` 改成 `-Height`
3. **删**:`ResizeDIBSection` 里那段"临时纯色"
4. **加**:`uint8` / `uint32` 等 typedef
5. **加**:`internal void RenderWeirdGradient(int XOffset, int YOffset)`(逐像素写 B/G 通道)
6. **改**:消息循环 → `while (Running) { while (PeekMessage(...PM_REMOVE)) {...} Render...; GetDC/StretchDIBits/ReleaseDC; ++XOffset; }`
7. **改**:`win32UpdateWindow` 改成整窗刷新(`0, 0, Width, Height`),`WM_PAINT` 里仍然可以调它(或者干脆交给主循环)

> ⚠️ 别忘了 `build.bat` 是 **`-W4 -WX`**(警告当错误):上面的 `uint8 Blue = (uint8)(X + XOffset);` 这种**显式转换**是必须的,不然会有"丢数据"的警告 → 直接编不过。

---

## 附录:本集新出现的 API / 常量速查

| 名字 | 作用 | 来自 |
|---|---|---|
| `VirtualAlloc` | 向系统**直接**要内存(按页) | kernel32 |
| `MEM_COMMIT` / `MEM_RESERVE` | 提交(真给)/ 保留(只占位) | winnt.h |
| `PAGE_READWRITE` / `PAGE_NOACCESS` | 内存保护属性 | winnt.h |
| `VirtualFree` / `MEM_RELEASE` | 释放整块内存还给系统 | kernel32 |
| `VirtualProtect` | 改内存保护属性(调试期抓悬空指针用) | kernel32 |
| `PeekMessage` | **非阻塞**取消息 | user32 |
| `PM_REMOVE` | 取走消息(而不是留着只看) | winuser.h |
| `GetDC` / `ReleaseDC` | 向窗口借/还一块画布(DC),**必须成对** | user32 |
| `uint8` / `uint16` / `uint32` | 明确位宽的无符号整数 | `stdint.h` 或自己 typedef |
| `-FC` | cl 的参数:报错信息里带**完整文件路径** | 编译器 |

### 顺带:他提到的两个"下一步"

- **光标**:窗口上现在是沙漏,因为没设置光标。游戏要自己画光标,这块留到以后处理。
- **性能测量**:他现在测不了(OBS 桌面捕获拖慢了机器),要等采集卡 —— 所以"这套老办法到底够不够快"这个问题的答案还没揭晓。
