# Day 003 —— Allocating a Backbuffer(分配后备缓冲)

> **素材**:`C:\Users\thoth\Downloads\[Mira] Handmade Hero Day 003 - Allocating a Backbuffer.txt`
> (3255 行字幕,约 1 小时;本文件按内容重排,不是逐句翻译)
> **本文件**:`D:\HandMade\explain\.HandmadeHeroExplain\Day 003 - Allocating a Backbuffer.md`
> **一句话总结**:**今天拿到了属于自己的那块像素内存** —— 从此渲染由我们自己写,Windows 只负责把它显示出来

---

## 一、这一集干了什么(总览)

分成两半:

| 时间段 | 内容 |
|---|---|
| 0:00 ~ 22:20 | **收尾**:修好"窗口关不掉"、定下代码风格约定(`static` 的三个宏)、反 RAII 的一段论述 |
| 22:20 ~ 58:00 | **正题:分配后备缓冲**(DIB section)—— 本集核心 |
| 58:00 ~ 结束 | Q&A(约 30 分钟) |

关键结论:**他把 backbuffer 拿到手了,但今天没来得及往里画**("doing all the work and not getting paid"),所以 **Day 4 才是真正开始画东西**。

---

## 二、前半:让窗口能正常关闭

### 1. `GetMessage` 的返回值为什么写 `> 0`

MSDN 明确写着:

| 返回值 | 含义 |
|---|---|
| `> 0` | 取到一条消息 |
| `0` | 收到了 **`WM_QUIT`** —— Windows 在告诉你"该退出了" |
| `-1` | 出错(也该退出) |

所以判断 `> 0` 才继续派发,其余一律跳出循环。

### 2. `PostQuitMessage` 与 `WM_QUIT`

想让循环退出,只要**往自己的消息队列里塞一条 `WM_QUIT`** 就行 —— 这正是 `PostQuitMessage(exitCode)` 干的事。`exitCode` 是个能顺带夹带出去的数字(游戏基本不关心,他反复强调"我们什么都不传递")。

于是 `WM_CLOSE` 里调一次 `PostQuitMessage(0)`,消息循环自然结束。

### 3. 他的一段"反 RAII"论述(值得一读)

面向对象背景的人看到他会说:"你 `CreateWindowEx` 了却没 `DestroyWindow`,这不干净。"

他的回答是:

- 他**自己曾经那样写了三年**("It's the only way I programmed"),所以不是不懂;
- 但后来发现:**资源应该"成批地获取和释放"(in aggregate / in waves)**,而不是一个资源一个资源地盯;
- **Windows 在进程退出时会替你清理所有窗口、句柄、内存** —— 这句话是重点;
- 所以在退出前手动逐个销毁,是**在浪费用户的时间**(你有没有遇到过"点了关闭,程序半天不退"的软件?一个主要成因就是这个);
- 我们的窗口在游戏运行期间**本就一直存在**,不需要销毁。

他也坦白了"什么时候该对称、什么时候不该"以后还会再讲(而且承认这段是**占位代码**,以后要重写)。

### 4. 最后采用的方案:一个 `running` 全局标志

除了"在 `WM_CLOSE` 里 `PostQuitMessage`",他更推荐另一种:把无限循环改成

```c
while (running)          // running 是一个全局布尔
{
    ...
}
```

然后在 `WM_CLOSE` / `WM_DESTROY` 里把 `running` 设为 `false`。

好处:以后可以在关闭前弹一个"你确定要退出吗?(进度会丢失)"的对话框 —— 这正是他说的"我们不一定想真的立刻退出"。

> `WM_DESTROY` 那个分支他打算以后当成"意外错误"处理(窗口被我们没预料到的原因销毁了 → 打标记 → 以后重建窗口)。

### 5. `static` 有三个含义,他改成三个宏

`static` 在 C 里是个"坏关键字",因为位置不同意思完全不同。他用 `#define` 拆成三个能搜得到、能看懂的名字:

| 他的宏 | 替代 | 含义 |
|---|---|---|
| `local_persist` | 函数内的 `static` 局部变量 | 值在离开作用域后**保留**(注意:只在**第一次**执行时初始化) |
| `global_variable` | 文件内的 `static` 全局变量 | 全局变量。**`static` 全局会自动清零**,不用手动初始化 |
| `internal` | 函数前的 `static` | 这个函数**只在本文件(translation unit)内可见**,别的文件不能调用 |

**为什么要这么做**:这样他可以一键搜索 `local_persist`,把"我不喜欢的"局部持久变量一个个清掉;还能一眼看出"谁在改这个全局"。

顺便他解释了之前 `WM_PAINT` 里那个 `static DWORD Operation` 的原理:**加了 `static` 之后,初始化只执行一次**,所以它才能记住上一次是黑还是白 —— 而这一集他把它**整个拆掉了**(改成真正的函数 + 全局状态)。

### 6. 现场事故:`#define` 顺序写反

他边讲边写,把宏写反了 → 编译报 **`static macro redefinition`**。他借机讲了一遍:

```
#define 你要定义的新名字   它要被替换成的东西
        ↑ 顺序绝对不能反
```

---

## 三、正题:分配后备缓冲(DIB section)

### 1. 总体思路

他想让 **GDI 只做一件事:把我们自己渲染好的位图显示出来**。GDI 是 Windows 的绘图 API,而我们的目标是"一切自己写",所以和 GDI 的接触面越小越好。

```
我们的渲染器  ──写像素──▶  backbuffer(我们自己的一块内存)
                                  │
                           Windows 负责拷贝
                                  ▼
                              窗口(屏幕)
```

他说这其实是"回到没有 3D 显卡的年代":那时候所有人都得自己写渲染器,再把缓冲区交给显卡显示。

### 2. 分成三个函数

| 函数 | 什么时候调 | 干什么 |
|---|---|---|
| `win32ResizeDIBSection(width, height)` | **每次 `WM_SIZE`** | 重建一块尺寸正确的后备缓冲 |
| `win32UpdateWindow(HDC, HWND, X, Y, W, H)` | **`WM_PAINT` 里**(`BeginPaint`/`EndPaint` 之间) | 把缓冲区里那一块贴到窗口上 |
| `win32CreateDIBSection(...)` | 被上面那个调用 | 真正去问 Windows 要内存 |

> 命名前缀 `win32` 是他刻意的习惯:Windows 自己有个叫 `UpdateWindow` 的函数,不加前缀名字会撞车 —— **"Windows 有太多名字可以撞了"**。

### 3. `GetClientRect` —— 拿"能画的区域"

`WM_SIZE` 只是告诉你"窗口尺寸变了",但**没说新尺寸是多少**。所以要自己问:

```c
RECT ClientRect;
GetClientRect(Window, &ClientRect);      // 注意是 LPRECT(指针)
int Width  = ClientRect.right  - ClientRect.left;
int Height = ClientRect.bottom - ClientRect.top;
```

**为什么是 `GetClientRect` 而不是 `GetWindowRect`**:后者给的是**整个窗口**(含标题栏、边框、阴影),前者给的只是**你能画进去的那块客户区**。边框那些都由 Windows 管,我们不该碰。

> 他提了一句:等以后全屏了就没这个问题,因为全屏没有边框。

### 4. DIB 是什么 / `CreateDIBSection` 的 6 个参数

**DIB = Device Independent Bitmap(设备无关位图)** —— Windows 对"一块你可以往里写、它也能拿来显示的位图"的叫法。

```c
HBITMAP CreateDIBSection(HDC hdc, const BITMAPINFO *pbmi, UINT usage,
                         void **ppvBits, HANDLE hSection, DWORD offset);
```

| 参数 | 他填什么 | 说明 |
|---|---|---|
| `hdc` | 一个"和屏幕兼容"的 DC(见下) | 它要按这个 DC 的格式建位图 |
| `pbmi` | `&BitmapInfo` | 描述这块位图长什么样的结构(下一节) |
| `usage` | `DIB_RGB_COLORS` | RGB 还是调色板 |
| **`ppvBits`** | `&BitmapMemory` | **★ 这就是今天要的东西**:Windows 把"它给你分配的那块内存的地址"写回这里 |
| `hSection` | `0` | 用于"把位图放进文件/内存映射"的高级玩法,我们不用 |
| `offset` | `0` | 配 `hSection` 用的,忽略 |

**返回值**:`HBITMAP`(位图句柄);**失败返回 NULL**。

### 5. `BITMAPINFO` 与 `BITMAPINFOHEADER` 逐字段

```c
typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;    // 真正要填的就是这个
    RGBQUAD          bmiColors[1]; // 调色板(RGB 模式下用不到)
} BITMAPINFO;
```

`bmiColors` 那个"长度 1"是**老式 C 的变长数组技巧**,他明确说"我不推荐你写这种代码,但 Windows 就这么写的"。

我们**只填 `bmiHeader`**:

| 字段 | 填什么 | 为什么 |
|---|---|---|
| `biSize` | `sizeof(BITMAPINFOHEADER)` | ⚠️ **只算头,不算整个 `BITMAPINFO`**。MSDN 里那句"程序可以用 `biSize` 定位颜色表"就是线索:颜色表在头之后,说明这个 size 指的是头的大小 |
| `biWidth` | 宽度 | 来自 `GetClientRect` |
| `biHeight` | 高度 | 同上 |
| `biPlanes` | **`1`** | "位平面"是老概念(红/绿/蓝各一个平面),现在必须填 1 |
| `biBitCount` | **`32`** | **24 位就够**(R/G/B 各 8 位),他故意要 32 是为了 **DWORD 对齐** |
| `biCompression` | `BI_RGB` | **不压缩**。我们要的是"写得快、贴得快",任何压缩都是负担 |
| `biSizeImage` | `0` | 只在有压缩时才有意义 |
| `biXPelsPerMeter` / `biYPelsPerMeter` | `0` | 只有打印输出才关心(每米多少像素) |
| `biClrUsed` | `0` | 调色板里用了多少色 —— 我们没有调色板 |
| `biClrImportant` | `0` | 同上 |

### 6. 调色板 vs 直接 RGB

- **调色板(palettized)**:缓冲区里存的是"索引",再去一张颜色表里查真正的 RGB。**当年为了省内存**(8/16 位当 24 位用)。
- **直接 RGB**:每个像素自己带 RGB 值。**我们现在用这个** —— 方便得多。
- 他说调色板的日子基本过去了(偶尔在纹理压缩里复活),我们不用管。

### 7. 光栅操作码(raster op)

`StretchDIBits` 最后一个参数决定"对这个目标区域做怎样的位运算"(XOR / AND / 直接覆盖……)。我们要的就是**直接覆盖**,所以用:

```c
SRCCOPY      // 把源直接拷过去,不做任何花招
```

### 8. `StretchDIBits` 的参数(以及为什么选它不选 `BitBlt`)

```c
int StretchDIBits(HDC hdc,
                  int xDest, int yDest, int DestWidth, int DestHeight,   // 目标矩形
                  int xSrc,  int ySrc,  int SrcWidth,  int SrcHeight,    // 源矩形
                  const VOID *lpBits, const BITMAPINFO *lpbmi,
                  UINT iUsage, DWORD rop);
```

**为什么选 `StretchDIBits`**:它**允许缩放**(源矩形和目标矩形可以不一样大)。现在两边尺寸相同(1:1 拷贝),但**以后要用固定分辨率渲染、再放大到窗口尺寸**,那时就必须靠它。所以现在就定下来。

参数对应关系:

| 参数 | 来自哪 |
|---|---|
| `hdc` | `BeginPaint` 给的那个 DC(他把 DC 一路传进来,而不是重新 `GetDC`) |
| 目标矩形 | **`Paint.rcPaint`** —— Windows 要你重画的那块("脏区域") |
| 源矩形 | `(0, 0, Width, Height)` —— 我们缓冲区里对应的那部分 |
| `lpBits` / `lpbmi` | `BitmapMemory` 和 `BitmapInfo` |
| `iUsage` | `DIB_RGB_COLORS`(必须和创建时一致) |
| `rop` | `SRCCOPY` |

### 9. 内存管理:几个必须搞对的地方

| 要点 | 说明 |
|---|---|
| **每次 `WM_SIZE` 都会调 resize** | 如果每次都分配新内存而不释放 → **内存泄漏**。所以创建新 DIB 之前必须先 `DeleteObject(旧的位图句柄)` |
| 释放顺序的纠结 | 他现场纠结"**先释放再创建**"还是"**先创建再释放**"(万一新分配需要旧内存腾出来?)。最后写了个 `// TODO: bulletproof this` —— 先释放,以后再改得更健壮 |
| **`DeleteObject`** | GDI 的删除很统一:**所有 GDI 对象都传给 `DeleteObject`**,它自己判断类型(没有 `DeleteBitmap` 这种函数)。而且**位图句柄本身就管着那块内存**,句柄一删内存就释放 |
| **`CreateCompatibleDC(0)`** | 传 0 = "给我一个和**屏幕**兼容的 DC"。为什么需要它:创建 DIB 时必须有个 DC 告诉 Windows"按这个设备的格式来"。搞反直觉的点是:**我们并不是用它来画** —— 只是拿它当"格式参照",而且**建好就一直留着**(不用每次释放) |
| 持久状态放哪 | `BITMAPINFO BitmapInfo`、`void *BitmapMemory`、`HBITMAP BitmapHandle`、`HDC BitmapDeviceContext` 都被提成**全局变量**(他还说以后会想办法去掉全局) |

### 10. 他在调试器里的验证(值得学的习惯)

1. **单步跟进 resize**:看到 `GetClientRect` 返回的矩形、算出的宽高(那次的窗口是 **1424×729**)、`BitmapInfo` 各字段的值(32bpp、1 个平面、`biSize` 正确)、拿到的位图句柄和内存指针;
2. **当场抓到一个 bug**:`if (BitmapDeviceContext)` 写错了,应该是 **`if (!BitmapDeviceContext)`**(没有才创建)—— 他说"**单步跟踪的好处就是能立刻抓到这种愚蠢错误**";
3. **用任务管理器查内存**:反复拖动窗口改变大小,盯着**工作集(working set)**看它会不会一直涨 —— 涨了就是泄漏。这一招他形容为"写这种精致(且容易出错)代码时必须做的复查"。

**最终结果**:内存拿到了,但**还没往里画任何东西**。他用了句自嘲:"把活干完了却没拿到报酬。"

---

## 四、Q&A 要点(约 58 分钟之后)

| 问题 | 他的回答 |
|---|---|
| **"`StretchDIBits` 只收一个像素指针,那我不需要 `CreateDIBSection`,自己 `malloc` 一块内存不行吗?"**(观众 sSilvan) | 他持怀疑态度:那块内存的**填充/对齐方式是 Windows 规定的**,而且必须和位图句柄关联。但他也不确定,**承诺去问懂 Windows 内部的人**,并写了 TODO。后来 sSilvan 又说"翻了 `StretchDIBits` 的源码,直接传自己的缓冲区看起来是可以的,只是**如果不是 DWORD 对齐它会先做一份本地拷贝**" → Casey 很高兴:那我们可以自己分配、把 `CreateDIBSection` 那一套全删掉,**但要测**(而且提醒"能跑通 ≠ 适合发布") |
| 会用 `Sleep` 吗 | 会。当游戏循环一圈跑得太快时,用它把时间让回给系统,免得把 CPU 烧到 100% |
| 怎么让 VS 配合你的外部构建 | `devenv /debugexe build\win32_handmade.exe`;再把"工具 → 选项 → 启动"设成"上次加载的解决方案",以后就不用手输了 |
| 别的窗口系统会不会没那么疯 | 不会。"我不会替 Windows API 的设计辩护 —— 它本该只要一句'我是个游戏,给我一块后备缓冲'就够了"。但他补了一句:现在新东西也未必更好(点名 HTML5 的音频 API),甚至**Windows 自己还在越改越难用** |
| 还有更详细的 C 入门系列吗 | 想做,但嗓子扛不住;建议先看现有的 Intro to C,并**读 K&R《C 程序设计语言》**、多练 |
| **`void *` 到底是什么** | "**我不在乎它指向什么类型的指针**"。因为 C 没有重载,`CreateDIBSection` 没法写成"你可以给我 `unsigned short*`/`unsigned int*`/`unsigned char*` 都行",所以只能用 `void *`。以后我们会把它**按需要转成两种类型**:`char *`(8 位,用来**做指针算术、定位到某个像素**)和 `unsigned int`(32 位,用来**一次性写一个 RGBA 像素**) |
| **为什么 `CreateDIBSection` 要传"指针的指针"(`&BitmapMemory`)** | 因为 C 函数**只能有一个返回值**,而它既要返回位图句柄、又要返回内存指针 → 只能"你把地址给我,我往里写"。他说这是很常见的技巧,**但他本人更喜欢另一种写法**:
```c
struct win32_dib_section_result {
    HBITMAP Handle;
    void   *Memory;
};
// 直接 return 结构体(C 允许按值返回结构体),更清楚,不用取地址
```
观众(Shawn Barrett,写 *Thief* 渲染器的人)提醒:**ANSI C 之前的 C 不允许返回结构体** —— Casey 说这大概就是为什么当年这个习惯没流行起来 |
| **"C 里非 0 为真,为什么函数却用 0 表示成功?"** | 这是 C 运行库的约定:**错误码 0 = 没出错**。副作用很坑:`if (Result)` 看起来像"成功了吗",其实是在问"**失败了吗**"。他还提到 **Berkeley sockets** 也是这个约定 |
| Emacs 里怎么快速按词跳 | 他用 **Ctrl + 方向键**(而不是 `M-f`/`M-b`)—— 就是他 `init.el` 里那套键位的由来 |
| I am GUI(IMGUI) | **这个词是他发明的**;以后在系列里会专门讲它的哲学,他说"很多讨论都没抓到重点,那是我的锅" |
| 为什么返回类型单独占一行 | C++ 时代的习惯(模板+函数名+参数太长),让函数名和参数能对齐 |
| 观众骂他不用现代 C++ | 他回应:RAII/template/STL **他都用过很多年**,而且那时候他的代码"一团糟";现在不用是有意识的取舍 |

最后:**明天(Day 4)整节课都用来往 bitmap 里画东西**,他说他"非常期待"。

---

## 五、和我们代码的关系 / 下一步

| Day 3 讲的 | 我们的状态 |
|---|---|
| 修好"窗口关不掉" | ✅ 已经修好了(09-19 注释掉 `WM_CLOSE` 那条 + `WM_DESTROY` 里 `PostQuitMessage`)。**他的 `running` 标志方案我们还没用**,两种都能关窗口,先不用改 |
| `WM_SIZE` 里调 resize | ⬜ 我们的 `WM_SIZE` 目前是**注释状态**,等有了 backbuffer 再恢复 |
| `WM_PAINT` 里贴图 | ⬜ 我们现在是 `PatBlt` 涂白涂黑,**下一步要换成 `StretchDIBits`** |
| `local_persist` / `global_variable` / `internal` 三个宏 | ⬜ 可选的风格约定;我们代码里还有一个 `static DWORD Operation`(就是他说要拆掉的那种) |
| `CreateDIBSection` + `BITMAPINFO` | ⬜ **这就是我们接下来要写的** |
| `GetClientRect` / `CreateCompatibleDC(0)` / `DeleteObject` | ⬜ 同上 |
| `void *` 当 backbuffer 指针 | ⬜ 以后按需转 `char *` / `unsigned int *` |

### 我们下一步(Day 4)要做的,按顺序

1. 加全局状态:`BITMAPINFO BitmapInfo`、`void *BitmapMemory`、`HBITMAP BitmapHandle`、`HDC BitmapDeviceContext`
2. 写 `win32ResizeDIBSection()`:填 `bmiHeader` → `CreateDIBSection` → `DeleteObject` 旧的
3. 恢复 `WM_SIZE`:调 `GetClientRect` + `win32ResizeDIBSection`
4. 写 `win32UpdateWindow()`:用 `StretchDIBits(..., SRCCOPY)` 把缓冲区贴到 `rcPaint` 那块
5. `WM_PAINT` 里把 `PatBlt` 换成 `win32UpdateWindow`
6. **然后才是爽的部分**:自己往 `BitmapMemory` 里写像素(先做个会动的渐变)

> ⚠️ 注意:我们的 `build.bat` 用的是 **`-W4 -WX`(警告当错误)**,所以新写的代码一旦有警告(比如未使用的变量、类型转换丢数据)就会**直接编译失败**。加代码时留意。

### 附:本集出现的 API 速查

| API | 作用 | 来自 |
|---|---|---|
| `GetClientRect` | 拿客户区(可绘制区域)矩形 | user32 |
| `CreateDIBSection` | **分配一块 Windows 认可的位图内存,并把指针给你** | gdi32 |
| `CreateCompatibleDC(0)` | 建一个和屏幕兼容的 DC(当"格式参照"用,长期持有) | gdi32 |
| `StretchDIBits` | 把 DIB(可缩放)贴到目标 DC | gdi32 |
| `DeleteObject` | 删除任意 GDI 对象(位图/画刷/笔…),位图会连带释放内存 | gdi32 |
| `PostQuitMessage` | 往队列里塞一条 `WM_QUIT` | user32 |
| `GetMessage` | 取消息(0 = `WM_QUIT`,−1 = 出错) | user32 |
| `SRCCOPY` / `BI_RGB` / `DIB_RGB_COLORS` | 光栅操作码 / 不压缩 / 直接 RGB 三个常量 | wingdi.h |
