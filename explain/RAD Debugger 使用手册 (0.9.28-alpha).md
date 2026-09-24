# RAD Debugger 使用手册(v0.9.28-alpha)

> **这份文档是什么**:RAD Debugger 的完整使用说明,外加**针对我们这个 Handmade Hero 项目**的具体菜谱。
>
> **装在哪**:`C:\THOTH\RuanJian\raddebugger-0.9.28-alpha\`(2026-09-21 装的,压缩包 `raddbg-v0.9.28-alpha.zip` 也留在同目录)
>
> **事实来源**(凡是标了记号的都要按这个看):
> - ✅ **实测** —— 我真的跑过 / 真的读过那个文件
> - 📖 **官方手册** —— 来自 `raddbg_readme.md`(随安装包,共 721 行)
> - ⚙️ **实际配置** —— 来自本机 `%APPDATA%\raddbg\default.raddbg_user`(它自己生成的,里面是**完整默认键位表**)
> - 🟡 **我的推断** —— 没验证,别当结论
>
> **版本**:`0.9.28-alpha` ✅(实测窗口标题就是 `The RAD Debugger (0.9.28 ALPHA)`)。注意这是 **alpha** —— 官方自己和压缩包名字里都写着,遇到怪毛病先怀疑它。

---

> # ⚠️ 先看这一条(2026-09-21 实测)
> **`raddbg.com` 会丢掉命令行里的程序路径。** 用它会打开一个"**没有目标程序**"的空调试器:
> 按 F5 只会内部报错,什么都调不了(日志里是 `run_entity_count: 0` + `kind: Error`)。
> **必须用 `raddbg.exe`**(实测:同名参数下它一切正常)。
> ```cmd
> C:\THOTH\RuanJian\raddebugger-0.9.28-alpha\raddbg.exe D:\HandMade\HandMade\build\win32_handmade.exe
> ```
> ⚠️ 陷阱:Windows 的 `PATHEXT` 把 `.COM` 排在 `.EXE` **前面**,所以你在命令行敲 `radbg` **永远**会选中那个坏的 `raddbg.com` —— 必须显式写 **`raddbg.exe`**。
## 0. 一页速查

| 我想…… | 怎么做 |
|---|---|
| 调试我们的程序 | `raddbg.exe D:\HandMade\HandMade\build\win32_handmade.exe` |
| 跑起来 / 继续 | **F5** |
| 在某行下断点 | 光标放那行 → **F9** |
| 单步(进函数) | **F11** |
| 单步(不进函数) | **F10** |
| 跳出当前函数 | **Shift+F11** |
| 全部停住(程序在疯跑时) | **Ctrl+Shift+X** 或 **Pause** |
| 全部杀掉重来 | **Shift+F5**;重新开始 = **Ctrl+Shift+F5** |
| 找任何命令 / 设置 / 窗口 | **F1**(Palette) |
| 看某个变量什么值 | 打开 Watch 标签,输表达式;或光标放变量上按 **Alt+W** |
| **把后备缓冲直接看成图片** | Watch 里输 `bitmap(GlobalBackBuffer.Memory, GlobalBackBuffer.Width, GlobalBackBuffer.Height, fmt=bgra8)` ⭐ |
| 看崩溃时的调用栈 | 崩了它自动停住 → 看 **Call Stack** 标签 |
| 改字号 | **Alt+=** / **Alt+-** |

---

## 1. 启动与安装位置

### 1.1 目录里有什么

```
C:\THOTH\RuanJian\raddebugger-0.9.28-alpha\
├── raddbg.com        ← ⚠️ 控制台包装器:会丢参数,别用它(见 1.3)
├── raddbg.exe        ← ★★ 命令行入口 + GUI 本体(用这个!)
├── radbin.exe        ← 二进制工具(查 PE / 转换等)
├── radlink.exe       ← 它自带的链接器
├── raddbg_markup.h   ← 源码里埋调试钩子的头文件(见第 8 节)
├── raddbg_readme.md  ← 官方手册(721 行,有问题先查它)
└── *.pdb             ← 它自己的调试符号
```

### 1.2 三种启动方式

```cmd
:: ① 只开调试器,再在里面加目标
C:\THOTH\RuanJian\raddebugger-0.9.28-alpha\raddbg.exe

:: ② 直接带上要调的程序(推荐)
C:\THOTH\RuanJian\raddebugger-0.9.28-alpha\raddbg.exe D:\HandMade\HandMade\build\win32_handmade.exe

:: ③ 带程序 + 给它命令行参数
raddbg.exe my_program.exe --foo --bar --baz
```

📖 **参数解析规则(会咬人,务必记)**:
- 看到**第一个不是 `-` / `--` 开头**的参数之后,后面的所有东西都算**目标程序的参数**,不再算调试器的参数。
- 也就是说 `raddbg test.exe --auto_run` 里的 `--auto_run` 是**传给 test.exe 的**,不是给调试器的!
- 想显式断开,用单独的 `--`:`raddbg -- test.exe`。

### 1.3 ⚠️⚠️ 最重要的坑:`raddbg.com` 不能用,要用 `raddbg.exe`

安装包里有两个入口:

| 文件 | 大小 | 能不能带目标程序 |
|---|---|---|
| `raddbg.exe` | 4,720,800 字节 | ✅ **可以** —— GUI 本体,直接收命令行 |
| `raddbg.com` | 219,808 字节 | ❌ **不行** —— 控制台包装器,会**丢掉程序路径** |

**实测证据(2026-09-21)**:

| 启动方式 | 结果 |
|---|---|
| `raddbg.exe --auto_run D:\...\win32_handmade.exe` | ★ 目标进程出现,几秒内吃掉几秒 CPU —— **完全正常** |
| `raddbg.com --auto_run D:\...\win32_handmade.exe` | ✗ 目标进程**从未出现**;换真实控制台窗口、换 C 盘路径、甚至给全权限,都一样 |
| 内部日志 `%APPDATA%\raddbg\logs\ctrl_thread.raddbg_log` | `user2ctrl_msg:{kind:"Launch"}` → `run_entity_count: 0` → `dmn_event: kind: Error`(错误串是空的) |
| 对照组:`cdb.exe`(微软调试器)调同一个 exe | ✅ 正常附加(有完整 ModLoad 列表) |

**为什么会踩**:Windows 的 `PATHEXT` 环境变量把 `.COM` 排在 `.EXE` **前面**,所以在命令行敲 `radbg` / `raddbg` 时,系统**优先**执行 `raddbg.com` —— 也就是说**默认那条路就是坏的那条**。

**三种修法**(任选,② 需要你同意改安装目录):

1. **永远显式写 `raddbg.exe`**(零风险,立刻可用);
2. 把 `raddbg.com` 改名(例如 → `raddbg_console_wrapper.com.bak`)—— 之后敲 `radbg` 就会落到 `raddbg.exe`;
3. 写一个 `radbg.bat` 包一层,里面调用 `raddbg.exe %*`。

> 另外注意:`--auto_run` 只是"启动后立刻跑"。**不带它也不会自动跑** —— 要用 F5 手动开始(实测:`launch_and_run` 这条命令等价于 F5,一样能跑起来)。
---

## 2. 第一次打开:界面长什么样

✅ 实测(2026-09-21,用我们的 `win32_handmade.exe` 当目标启动):深色界面,窗口 1721×927(它会记住尺寸,下次更大)。

- **菜单栏**:`File` `Window` `Panel` `Tab` `Targets` `Control` `Help`
- **标签页**(它内部叫 tabs):`Disassembly`、`Getting Started`、`win32_handmade.cpp`(它自动打开了我们的源文件)
- **左侧面板的标签**:`Disassembly`、`Breakpoints`、`Watch Pins`、`Output`、`Memory`
- 底部有 **Locals / Registers / Globals / Types / Procedures** 和 **Call Stack / Modules**
- 右侧是 **Targets** 和 **Threads / Processes / Machines**

完整默认布局见 **附录 B**。

> ⚠️ **一个重要的默认行为**:带目标启动后它**不会自动运行**程序。✅ 实测:启动后 `win32_handmade.exe` 进程并不存在 —— 它停在"等你操作"的状态。要跑就按 **F5**;想一动不动停在入口点则用 `--auto_step`。

---

## 3. 四个核心概念

### 3.1 窗口 / 面板 / 标签(Windows / Panels / Tabs)

📖 官方设计里**没有"特殊窗口"**:

- **窗口**被切成若干**面板**(panel),面板之间**不重叠**;
- 每个面板里可以放多个**标签**(tab),同一时刻显示一个;
- 标签可以**在面板之间拖来拖去**;
- 任何面板都能放任何标签 —— 没有哪个位置是"专用"的。

这意味着:**布局随便你改**。改乱了就 `F1` → `Reset To Default Panel Layout`(还有 Compact / Simple 两种预设)。

### 3.2 目标(Target)

📖 一个 **target** = 「一个 exe」+「怎么启动它」(命令行参数、工作目录),还可以有:
- **自定义标签**(label)
- **自定义入口点函数名**(默认认 `main`、`WinMain` 等)
- **启用 / 禁用**状态

要点:
- 可以同时有多个 target;`F5`、`Shift+F5` 这类操作作用于**所有已启用**的 target。
- **从命令行带进来的 target 是临时的**,关掉调试器就没了。想留住:到 **Targets** 标签里,点那一行的 **`Save To Project`** 按钮 —— 之后就写进项目文件,下次不用再敲命令行。⭐

### 3.3 视图 / View(⚠️ 这是 RAD Debugger 最特别的地方)

📖 普通调试器给你一个"值";RAD Debugger 让你**改变这个值的呈现方式**。写法**长得像函数调用**:

```
bitmap(GlobalBackBuffer.Memory, 1280, 720, fmt=bgra8)
hex(my_int)
array(ptr, 64)                    // 把 ptr 当成 64 个元素的数组看
slice(my_slice)                   // 认"基址指针 + 数量"的结构
rows(s, x, y, z)                  // 只看结构体里的 x y z 三个成员
omit(s, x, y, z)                  // 看除了 x y z 之外的所有成员
digits(bin(my_int), 32)           // 二进制、至少显示 32 位
```

支持的 view(📖 全部,共 20 个):

| 分类 | view |
|---|---|
| 进制 | `bin` `oct` `dec` `hex` `digits(expr, num)` |
| 显示开关 | `raw`(忽略所有 view)、`no_string` `no_char` `no_addr` |
| 结构 | `rows(expr, ...)` `omit(expr, ...)` `columns(expr, ...)` `sequence(expr)` |
| 数组/切片 | `array(expr, count)` `slice(expr)` |
| 范围 | `range1(expr, min, max)`(会生成**滑块** UI) |
| **专用可视化** ⭐ | **`bitmap(expr, width, height, [fmt=...])`**、`text(expr,[lang=...])`、`disasm(expr,[size=...])`、`memory(expr,[size=...])`、`color(expr)` |

> **`bitmap` 对我们意义重大** —— 见第 12.1 节:你可以**在调试器里直接看到后备缓冲的那张图**。

### 3.4 断点(Breakpoints)

📖 三种下法:
1. **源码行**上(会解析成代码地址;那一行没代码就用同文件里最近的一行)
2. **任意地址**上(反汇编视图里、或用表达式如函数名)
3. **数据上**(地址断点可以指向**数据**而不是代码 —— 读/写/执行时停,走**硬件数据断点**,字节数只能填 **1 / 2 / 4 / 8**)

外加:
- **停止条件(stop condition)**:命中时先算这个表达式,**非 0 才真的停**;
- **命中计数(hit count)**:每次真的停下来就 +1。

---

## 4. 完整快捷键表

⚙️ 下面这张表**不是我从手册里挑的**,是从本机实际生成的 `%APPDATA%\raddbg\default.raddbg_user` 里**逐条读出来的完整默认键位**(手册正文只列了 17 个)。

### 4.1 执行控制(最常用)

| 快捷键 | 命令 | 作用 |
|---|---|---|
| **F5** | `run` | 运行 / 继续 |
| **Shift+F5** | `kill_all` | 杀掉所有进程 |
| **Ctrl+Shift+F5** | `restart` | 全杀后重新启动所有启用的 target |
| **F11** | `step_into` | 单步(会进函数) |
| **F10** | `step_over` | 单步(不进函数) |
| **Shift+F11** | `step_out` | 执行到当前作用域退出 |
| **Alt+F11** | `step_into_inst` | **指令级**单步(进) |
| **Alt+F10** | `step_over_inst` | **指令级**单步(不进) |
| **Ctrl+F10** | `run_to_line` | 跑到光标那一行 |
| **Ctrl+Shift+F10** | `set_next_statement` | **把 IP 设到光标处**(跳过/重跑一段) |
| **Ctrl+Shift+X** 或 **Pause** | `halt` | 全部停住 |
| **Shift+F6** | `attach` | 附加到已经在跑的进程 |

### 4.2 断点与监视

| 快捷键 | 命令 | 作用 |
|---|---|---|
| **F9** | `toggle_breakpoint` | 光标处下/取消断点 |
| **Shift+F9** | `add_address_breakpoint` | 加**地址**断点 |
| **Ctrl+Shift+F9** | `add_function_breakpoint` | 加**函数名**断点 |
| **Ctrl+F9** | `toggle_watch_pin` | 在光标处加/取消 **watch pin** ⭐ |
| **Alt+W** | `toggle_watch_expr_at_cursor` | 把光标处的表达式加进 Watch |
| **Ctrl+D** | `toggle_watch_expr_at_mouse` | 把鼠标指着的表达式加进 Watch |

### 4.3 面板与标签

| 快捷键 | 作用 |
|---|---|
| **Ctrl+T** | 新开一个标签(Open Tab) |
| **Ctrl+W** | 关标签 |
| **Ctrl+Tab / Ctrl+Shift+Tab** | 下一个 / 上一个标签 |
| **Ctrl+PageDown / Ctrl+PageUp** | 同上 |
| **Ctrl+Shift+PageDown / PageUp** | 把标签往右 / 左挪一格 |
| **Ctrl+Alt+T** | 当前标签的设置 |
| **Ctrl+Shift+Alt+↑ / ↓** | 标签栏放顶部 / 底部 |
| **Ctrl+, / Ctrl+Shift+,** | 下一个 / 上一个面板 |
| **Ctrl+Alt+方向键** | 按方向切面板 |
| **Ctrl+P / Ctrl+减号** | 向右 / 向下**切分**面板 |
| **Ctrl+Shift+Alt+P** | 关掉当前面板 |
| **Ctrl+2** | 轮转面板列 |
| **Ctrl+Enter** | 全屏切换 |
| **Alt+= / Alt+-** | 窗口字号 +1 / -1 |

### 4.4 导航与编辑

| 快捷键 | 作用 |
|---|---|
| **F1** / **Ctrl+Shift+P** | 打开 **Palette** ⭐ |
| **Ctrl+O** | 打开文件 |
| **Ctrl+I** | 从调试信息里打开源文件 |
| **Alt+O** | 头文件 ↔ 实现文件 互跳(partner file) |
| **Ctrl+G** / **Alt+G** | 跳到行 / 跳到地址 |
| **Ctrl+J** | Go To Name(文件名、符号名都能跳) |
| **F12** | Go To Name At Cursor(光标处的名字) |
| **Alt+← / Alt+→** | 后退 / 前进 ⭐ |
| **F4** | 跳到当前选中线程所在位置 |
| **Ctrl+F** 或 `/` | 查找;`Ctrl+R` 反查;F3 / Ctrl+F3 下一个 / 上一个 |
| **Ctrl+Z / Ctrl+Y** | 撤销 / 重做 |
| **F2** | 编辑当前项 |
| **Enter / 空格** | 确认;`Esc` 取消 |
| **Ctrl+L** | 锁定 / 解锁当前项 |
| **Alt+D** | 聚焦菜单 |
| **Ctrl+Shift+M** | 在日志里打一个 marker(配合 `log_marker`) |
| **Ctrl+Shift+Alt+D** | 开发者菜单 |

### 4.5 项目文件

| 快捷键 | 作用 |
|---|---|
| **Ctrl+Shift+N / Ctrl+Shift+O / Ctrl+Shift+S** | 新建 / 打开 / 保存 **项目文件** |

---

## 5. 命令系统:那个 F1 面板才是最全的菜单

📖 **整个调试器的操作几乎全都是"命令"** —— 界面上的菜单项本身就是命令的包装。所以:

> **任何时候你不知道某个操作怎么做 → 按 F1,搜命令。**

- **F1** 打开 **Palette**:里面能搜**命令、快捷键、设置、线程、进程、模块、类型**……
- 命令还可以**带参数**执行(比如 `add_breakpoint` 要你给位置)。
- 📖 命令名也是 **`--ipc` 模式的通信语言**(见第 10 节)。

⚠️ 这套设计有个副作用:**别指望在菜单里找全所有功能**。菜单只挂了一部分,Palette 才是全集。本文档第 0 节那些"没有快捷键"的操作,都是这么来的。

---

## 6. Watch 表达式

📖 Watch 里的表达式**大致遵循 C 语法**,但有一堆扩展:

```
pointer, 64                // 把 pointer 看成 64 个元素
array(pointer, 64)         // 同上,另一种写法
pointer, count
slice(some_slice_struct)
hex(my_int)   / my_int, x  // 十六进制
dec(my_int)   / my_int, d  // 十进制
bin(my_int)   / my_int, b  // 二进制
digits(bin(my_int), 32)
bitmap(base_pointer, width, height, fmt=rgba8)     ⭐
rows(some_struct, a, b, c)
omit(some_struct, a, b, c)
```

### 6.1 那些 `query:` 表达式是什么

⚙️ 默认布局里那些标签页其实都是 Watch,只不过表达式写的是 `query:xxx`:

| 标签 | 表达式 |
|---|---|
| Locals | `query:locals` |
| Registers | `query:registers` |
| Globals | `query:globals` |
| Types | `query:types` |
| Procedures | `query:procedures` |
| Call Stack | `query:call_stack` |
| Breakpoints | `query:breakpoints` |
| Watch Pins | `query:watch_pins` |
| 输出 | `query:output` |
| Targets / Threads / Processes / Modules / Machines | `query:targets` 等 |

**这条很有用**:如果你想要一个"同时看 Locals 和某个变量"的自定义标签,直接自己开个 Watch 写多行表达式就行 —— 它和内置标签是同一个机制,没有魔法。

---

## 7. 断点进阶(⚠️ 对我们项目尤其重要)

### 7.1 停止条件

📖 断点可以挂**条件表达式**,命中时先算它,**非 0 才停**。还可以看**命中计数**。

### 7.2 数据断点(硬件读写断点)

📖 地址断点可以指向**数据**,在**读 / 写 / 执行**时停。用法:在断点编辑器里填"从该地址起要检查几个字节"(**只能是 1 / 2 / 4 / 8**),再勾选读写执行。

> **这是抓"谁改了我的内存"的唯一实用手段。** 详见第 12.3 节。

### 7.3 `Break When Value Changes`(现成命令)

📖 有个命令叫 **`Break When Value Changes`(`break_when_value_changes`)**:对某个变量直接**加一个写数据断点**。比手动配地址断点省事 —— 在 Watch 里对着一个变量用它就行。

### 7.4 ⚠️ 我们的程序是个"疯跑的循环",断点会把你烦死

我们的主循环现在是:

```c
while (Running) { 排空消息; RenderWeirdGradient(...); blit; ++Xoffset; ++Yoffset; }
```

它**没有任何等待**,一秒能跑几千上万圈。所以:

| 现象 | 原因 | 怎么办 |
|---|---|---|
| 在 `RenderWeirdGradient` 里下断点,按 F5 后**几乎立刻又停住**,像卡死 | 每一帧都会命中,而它一秒钟有几千帧 | 用**条件断点**,或者先 `Halt` 再单步 |
| `Xoffset` 的数值每次看都不一样 | 帧计数在飞 | 用 `Xoffset % 256 == 0` 这类条件 |
| 想"只看一帧" | —— | 断在行上 + 条件 `Y == 0 && X == 0`(一个像素) |

**以后如果加了 `Sleep` / 计时,这个问题会好很多** —— 那也在我们的待办清单上(见 `explain\` 里的日志)。

---

## 8. 源码里埋调试钩子:`raddbg_markup.h`

✅ 实测:安装包里的 `raddbg_markup.h` 提供了这些宏(📖 并在**没有调试器附加时全部编译成空操作**,所以可以一直留在代码里):

| 宏 | 作用 |
|---|---|
| `raddbg_is_attached()` | 有没有调试器挂着 |
| `raddbg_break()` | 在这里断下来(相当于代码里写死的断点) |
| `raddbg_break_if(expr)` | 条件成立才断 |
| `raddbg_watch(fmt, ...)` | 把值**推给**调试器显示(类似 printf,但进调试器) |
| `raddbg_pin(expr)` | 在代码里"钉"一个监视表达式 ⭐(编译期 no-op,由调试器 UI 读) |
| `raddbg_log(fmt, ...)` | 往调试器日志里写 |
| `raddbg_thread_name` / `raddbg_thread_color_rgba` | 给线程起名、上色(多线程时非常有用) |
| `raddbg_entry_point(...)` | 指定入口点(代替默认的 main / WinMain) |
| `raddbg_type_view(type, ...)` | **给某个类型注册默认的可视化方式**(一次注册,以后所有该类型的 Watch 都自动按它显示)⭐ |
| `raddbg_add_breakpoint(ptr,size,r,w,x)` / `raddbg_remove_breakpoint(...)` | 程序**自己**在运行时加 / 删数据断点 |
| `raddbg_annotate_vaddr_range(...)` | 给一段虚拟地址加注释(调试器里能看见) |
| `raddbg_load_module(ptr,size,name,dbg_path)` | JIT 场景下手动注册模块 |

> **`raddbg_type_view` 值得记住**:以后我们写出 `v2`(二维向量)、`color`、`bitmap` 这类类型时,可以注册一次可视化,以后 Watch 里就不用每次手写 view 了。

**怎么用**:`#include "raddbg_markup.h"` 即可。⚠️ 但注意我们是 `-W4 -WX`(警告即错误),引入第三方头文件**有可能带出警告**;真要引入,先单独编译一次看看,别直接塞进受版本管理的代码里(这一步得你同意)。

---

## 9. 命令行参数(📖 全部)

| 参数 | 作用 |
|---|---|
| `--help` | 打印帮助菜单 |
| `--user:<path>` | 指定**用户文件**路径(没有就创建) |
| `--project:<path>` | 指定**项目文件**路径(没有就创建) |
| `--auto_step` | 启动后**立刻单步**进入所有 target(停在入口点附近) |
| `--auto_run` | 启动后**立刻运行**所有 target |
| `--quit_after_success` 或 `-q` | 所有进程**以 0 正常退出**后,自动关掉调试器 |
| `--ipc` | 不开 GUI,把后面的文本当**命令**发给已在运行的调试器实例 |

⚠️ 再提醒一次**解析规则**:遇到第一个非 `-` 开头的参数后,调试器就停止解析自己的选项 —— 后面的全是目标的参数。

`--auto_run` + `-q` 组合起来 = **"跑一遍,正常结束就自己关掉"**,适合做成脚本化的冒烟测试:
```cmd
raddbg.exe --auto_run -q D:\HandMade\HandMade\build\win32_handmade.exe
```
⚠️ 🟡 但我们的程序**永远不会自己退出**(要你点 X 或者发 WM_QUIT),所以这两个开关对我们现在的程序意义不大 —— 等以后有了"自动跑 N 帧然后退出"的逻辑再用。

---

## 10. `--ipc`:让编辑器驱动调试器(以后接 Emacs 就靠它)

📖 用法:

```cmd
raddbg --ipc find_code_location "C:/foo/bar/baz.c:123:1"
```

- 它**不开新 GUI**,而是把 `find_code_location` 这条**命令**发给**已经在运行的那个**调试器实例,然后自己退出。
- 能发的是**全部命令**(第 5 节那套),只是要按文本编码。
- 📖 官方明确说这是**给编辑器和别的程序用的**。

> 🟡 **对我们的意义**:你现在用 Emacs + `M-m` 编译。以后想做到"编译 → 断点 → 在 Emacs 里跳转",靠的就是 `--ipc` + 命令名(附录 A)。**这是后期的事,现在不用管。**

---

## 11. 配置文件:user 文件 与 project 文件

📖 两份文件**格式相同**,调试器**自动保存**(不用手动存):

| | 用户文件 user | 项目文件 project |
|---|---|---|
| 默认路径 | `%appdata%/raddbg/default.raddbg_user` | `%appdata%/raddbg/default.raddbg_project`(或上次加载过的路径) |
| 存什么 | **文件路径映射**、窗口(显示器/位置/尺寸)、每个窗口的**面板布局和标签**、**键位**、主题颜色、字体 | **targets**、**断点**、**watch pins**、异常码过滤器 |
| 要不要进版本管理 | 一般**不要**(个人偏好) | **要** —— 它就是该进源码管理的那种东西 ⭐ |

✅ 实测:我启动过一次之后,本机已经生成了 `C:\Users\thoth\AppData\Roaming\raddbg\default.raddbg_user`(5420 字节),里面已经有**完整键位表**和当前面板布局,还有一行 `current_path: "D:/HandMade"`。

**要点**:
- 📖 两份文件都是**纯文本、可以手改**,改完它也会重新读。
- 📖 想"把用户文件里的东西挪到项目文件里"(或反过来),目前**只能手工搬文本**,UI 里没有这个功能(官方说计划做)。
- 📖 怀疑配置烂了 → 删掉整个 `%appdata%\raddbg` 文件夹即可(官方同时希望你顺手把文件发给他们报 bug)。
- 常用命令:`Open User`(**Ctrl+Alt+Shift+O**)、`Open Project`(**Ctrl+Alt+O**)、`User Settings`、`Project Settings`。

---

## 12. 针对 Handmade Hero 项目的实战菜谱 ⭐

### 12.1 把后备缓冲**直接看成一张图**(最有用的一招)

我们的后备缓冲是一块内存,长这样(见 `code\win32_handmade.cpp`):

```c
struct win32_offscreen_buffer { BITMAPINFO Info; void *Memory; int Width, Height, Pitch, BytesPerPixel; };
global_variable win32_offscreen_buffer GlobalBackBuffer;
```

内存序是 **蓝、绿、红、填充**(小端机器上寄存器里才是 `xx RR GG BB`)。所以:

1. 按 **Ctrl+T** 开一个新的 Watch 标签;
2. 输入:

```
bitmap(GlobalBackBuffer.Memory, GlobalBackBuffer.Width, GlobalBackBuffer.Height, fmt=bgra8)
```

3. 程序一停,你就能**在调试器里看见屏幕上那张渐变图**(而且不用切窗口、不会被别的窗口挡住)。

🟡 **关于 `fmt=bgra8`**:📖 手册给的默认是 `rgba8`、示例里用 `fmt=bgra8` 覆盖;我们的内存序是"蓝绿红填充",所以**推断**该用 `bgra8`。**如果颜色不对(比如红蓝互换),把 `fmt` 换掉试** —— 一眼就能看出来。

**更好用的一招(钉在代码上)**:把光标放在 `RenderWeirdGradient` 那一行 → 按 **Ctrl+F9** 建一个 **watch pin** → 在 **Watch Pins** 标签里把它的表达式改成上面那句。这样**只要停在这一行附近就自动显示图片**,不用每次手敲 ⭐

### 12.2 调试我们的 exe 的正确姿势

```cmd
C:\THOTH\RuanJian\raddebugger-0.9.28-alpha\raddbg.exe D:\HandMade\HandMade\build\win32_handmade.exe
```

**前提(重要)**:
- RAD Debugger 📖 **只支持 Windows x64 + PDB**。
- 我们的 `build.bat` 里有 **`-Zi`**(产出调试信息),链接后 `build\` 里是:
  - `win32_handmade.exe`(357,888 字节)
  - `win32_handmade.pdb`(4,378,624 字节)✅ 实测存在
- ⚠️ **exe 和 pdb 必须来自同一次构建**。所以:改完代码 → 先 `build.bat` → 再启动调试器。中途重编译过而调试器还开着,就会出现"行号对不上 / 源码错位"。
- ⚠️ **调试器开着时 `build.bat` 会失败** —— Windows 锁着正在运行的 exe(报 `cannot open win32_handmade.exe for writing`)。**先 Shift+F5 或关掉调试器**再编译。这条在 Day 005 视频里 Casey 也踩过 ✅(见 `Day 005` 讲解文档)。
- ⚠️ **用 VS 工程(`HandMade.sln`)编出来的 exe 在别的目录**(`HandMade\x64\Debug\HandMade.exe` + 同名 pdb)。**两条路线的产物不要混用**。

**建议**:如果你想固定下来(不用每次敲命令行),在 **Targets** 标签里点那个 target 的 **`Save To Project`** 按钮,并给它指定一个**项目文件**(比如 `D:\HandMade\HandMade\win32_handmade.raddbg_project`)。这样它还会记住断点。⚠️ **但那是往仓库里加文件,按 `NOTES.md` 的规矩得你点头**。

### 12.3 抓"谁把我的内存改坏了"—— 数据断点

回想 Day 005 那次翻车:结构体搬完了,`BytesPerPixel` 忘了初始化,于是 `VirtualAlloc(0, 0)` 返回空指针,写地址 `0x00000000` 当场崩。**这类 bug 用手动单步能查,但用数据断点是秒杀:**

```
① 在 Watch 里输入 GlobalBackBuffer.BytesPerPixel
② 对它用命令 Break When Value Changes（或在断点编辑器里指向它的地址、大小填 4、勾"写"）
③ F5 —— 只要有人写这个字段就会停下来
```

同样适用于:
- **谁把 `GlobalBackBuffer.Memory` 写成 0 了**
- **`Pitch` 什么时候被算错的** → `raddbg_add_breakpoint(&GlobalBackBuffer.Pitch, 4, 0, 1, 0)`
- 以后 `BitmapMemory` 越界写穿相邻结构时(填 8 字节检查范围)

📖 注意限制:硬件数据断点**字节数只能是 1 / 2 / 4 / 8**,而且可用的硬件槽位有限(CPU 一般就 4 个)。

### 12.4 条件断点:让疯跑的程序"只在我关心的时候停"

```
条件示例（我们的 RenderWeirdGradient 内层循环）：
  Y == 0 && X == 0            只想看每帧第一个像素
  Y == 100 && X == 200        只看某一个像素怎么写进去的
  Xoffset % 256 == 0          每 256 帧停一次
  Buffer.Memory == 0          只想在异常值上停
```

### 12.5 崩了怎么查(访问违例那类)

1. 它会在异常处**自动停住**;
2. 打开 **Call Stack** 标签看调用栈;点某一帧 → 寄存器和表达式都在**那一帧的上下文**里求值(📖);
3. 想看**汇编**:对源码行用 `Go To Disassembly`(命令),或直接看默认布局左上角的 **Disassembly** 标签;
4. 想看**原始内存**:用 `Memory` 标签,或 `memory(expr, size=...)`、`go_to_memory` 命令;
5. **越界 / 野指针**:用 `memory(...)` + `bitmap(...)` 结合起来看那块内存到底变成了什么。

### 12.6 附加到已经跑起来的进程

- **Shift+F6**(`attach`)附加到本机已在运行的进程;
- 或者 `Control` 菜单 / Palette 里搜 `attach`。

📖 也支持**注册为 JIT 调试器**(命令 `register_as_jit_debugger`)—— 注册之后**任何程序崩了都会弹出 RAD Debugger**。⚠️ 这是**系统级设置**,会影响整台机器上所有程序的崩溃行为 —— **想开之前先确认你愿意**。

### 12.7 多线程(以后)

等我们走到多线程渲染 / 模拟那一段(参考 `HANDMADE_HERO_终点与导航.md` 里的 Day 112–131、331–378),你会用到:

- **Threads** 标签:所有进程的所有线程;
- **Freeze Thread / Thaw Thread / Freeze Process**(逐线程冻结 —— 调试并发问题的关键工具);
- `raddbg_thread_name()` / `raddbg_thread_color_rgba()`:给线程起名上色,一眼分清谁是谁。

### 12.8 系统 DLL 的符号

我们的调用栈**一定会穿进 `user32.dll` / `gdi32.dll`**(`DispatchMessage`、`StretchDIBits` 都是它们)。想看到微软那些函数的**名字和参数**:

- 用命令 **`Download Module Debug Info`(`download_module_debug_info`)**,📖 它会从符号服务器下载该模块的调试信息;
- 不下载也能调,只是栈里那几个框会显示成地址而不是函数名。

---

## 13. 已知问题与注意事项

| 事项 | 说明 |
|---|---|
| ⚠️⚠️ **用 `raddbg.com` 会丢目标程序** | **必须用 `raddbg.exe`**。见 1.3 节的实测表。这是本项目踩过的最大一个坑。 |
| ⚠️ **中文注释的显示**(未完全确认) | ✅ 实测截图里,我们的 `win32_handmade.cpp` 中 **ASCII 部分正常**(`struct win32_offscreen_buffer`、`(32bpp)` 都能看清),但**中文注释疑似显示成方块 / 占位符**。🟡 读图工具给的判断**带不确定标记**,我第二次复核时视觉服务超时了,**所以这不算定论**。可能的原因:**① 它自带的等宽字体没有中文字形**(最可能);**② 编码解释问题**。补充事实:我们的源文件是 **UTF-8 无 BOM**(✅ 实测前 4KB 是合法 UTF-8),本机系统 ANSI 代码页是 **65001(UTF-8)**。<br>**怎么试**:`Alt+=` 调大字号看看是不是字形问题;`User Settings` 里找字体设置;或者把 `%APPDATA%\raddbg\default.raddbg_user`(纯文本)手改后重启调试器。 |
| ⚠️ **alpha 版** | 版本号自带 `-alpha`。官方 0.9.28 的 release notes 里就修了一堆崩溃(#893、#882、#877…)。用之前先接受"可能崩"。 |
| ⚠️ **它只支持 Windows x64 + PDB** | 官方原话。所以 **MinGW / gcc 编出来的 exe 没戏**(📖 说在做了)。我们的 `build.bat` 用的正是 MSVC + `-Zi`,✅ 合适。 |
| ⚠️ **调试器开着时编不过** | Windows 锁着运行中的 exe。先 `Shift+F5`。 |
| ⚠️ **权限** | 要调**管理员权限**的进程,你得**用管理员身份**启动调试器。它只做用户态调试(user-mode)。 |
| **性能** | 程序跑起来依然占满一个 CPU 核 —— 因为我们没做帧率控制,不是调试器的问题。 |
| **窗口老是置顶?** | 那是我验证脚本干的,不是它的默认行为。 |
| **它自己的配置在哪** | `%APPDATA%\raddbg\`(✅ 实测已生成 5420 字节的 `default.raddbg_user`)。删掉它就能恢复出厂设置。 |

---

## 14. 附录 A:常用命令名(`--ipc` / Palette 用的文本名)

📖 官方列出的命令里,挑出我们用得上的(格式:`界面名字`(`文本名`)):

**执行类**:`Launch and Run`(`launch_and_run`)、`Launch and Step Into`(`launch_and_step_into`)、`Run`(`run`)、`Continue`(`continue`)、`Halt`(`halt`)、`Restart`(`restart`)、`Kill`(`kill`)、`Kill All`(`kill_all`)、`Detach`(`detach`)、`Attach`(`attach`)、`Exit`(`exit`)

**单步**:`Step Into`(`step_into`)、`Step Over`(`step_over`)、`Step Out`(`step_out`)、`Step To Exit`(`step_to_exit`)、`Step Into (Assembly)`(`step_into_inst`)、`Step Over (Assembly)`(`step_over_inst`)、`Step Into (Line)`(`step_into_line`)、`Step Over (Line)`(`step_over_line`)、`Run To Line`(`run_to_line`)、`Run To Name`(`run_to_name`)、`Set Thread IP`(`set_thread_ip`)、`Set Next Statement`(`set_next_statement`)

**断点**:`Add Line Breakpoint`(`add_breakpoint`)、`Toggle Line Breakpoint`(`toggle_breakpoint`)、`Enable Breakpoint`(`enable_breakpoint`)、`Disable Breakpoint`(`disable_breakpoint`)、`Clear Breakpoints`(`clear_breakpoints`)、`Add Watch Pin`(`add_watch_pin`)、`Break When Value Changes`(`break_when_value_changes`)

**查看**:`Open Palette`(`open_palette`)、`Open`(`open`)、`Go To Line`(`goto_line`)、`Go To Address`(`goto_address`)、`Go To Name`(`goto_name`)、`Go To Name At Cursor`(`goto_name_at_cursor`)、`Go To Disassembly`(`go_to_disassembly`)、`Go To Source`(`go_to_source`)、`Go To Memory`(`go_to_memory`)、`Find Code Location`(`find_code_location`)、`Find Next` / `Find Previous`、`Switch To Partner File`(`switch_to_partner_file`)、`Show File In Explorer`(`show_file_in_explorer`)、`Search` / `Search Backwards`、`Clear Output`(`clear_output`)

**监视**:`Toggle Watch Expression`(`toggle_watch_expr`)、`...At Cursor`(`toggle_watch_expr_at_cursor`)、`...At Mouse`(`toggle_watch_expr_at_mouse`)、`Eval`(`eval`)、`State`(`state`)

**线程 / 进程**:`Select Thread`(`select_thread`)、`Freeze Thread` / `Thaw Thread`、`Freeze Process` / `Thaw Process`、`Freeze Local Machine` / `Thaw Local Machine`、`Select Unwind`(`select_unwind`)、`Up One Frame` / `Down One Frame`

**面板 / 标签**:`Open New Window`(`open_window`)、`Close Window`、`Toggle Fullscreen`、`Split Panel Left/Up/Right/Down`、`Rotate Panel Columns`、`Focus Next/Previous Panel`、`Close Panel`、`Focus Next/Previous Tab`、`Move Tab Left/Right`、`Duplicate Tab`、`Close Tab`、`Anchor Tab Bar To Top/Bottom`、`Selected Tab Settings`、`Reset To Default/Compact/Simple Panel Layout`

**配置**:`New/Open/Save/Write User|Project`、`User Settings`、`Project Settings`、`Open Recent Project`、`Set Current Path`、`Reset To Default Bindings`

**调试器自身**:`Download Module Debug Info`、`Load/Unload Debug Info`、`Register As Just-In-Time (JIT) Debugger`、`Open/Close Event Buffer`、`Toggle Developer Menu`、`Log Marker`、`Increase/Decrease Window|View Font Size`、`Bring To Front`、`Edit`、`Accept`、`Cancel`、`Lock` / `Unlock` / `Toggle Lock`

---

## 15. 附录 B:默认面板布局

⚙️ 从本机生成的用户文件里读出来的(不是猜的):

```
┌───────────────────────────────────────────────┬──────────────┐
│  左 25%             │  右 75%                  │  Targets     │  ← 0.15
│  Disassembly        │  Getting Started         ├──────────────┤
│  ─────────────────  │  win32_handmade.cpp      │  Threads     │
│  Breakpoints        │  （源码 / 文本视图）      │  Processes   │
│  Watch Pins         │                          │  Machines    │
│  Output             │                          │              │
│  Memory             │                          │              │
├─────────────────────┴──────────────────────────┤              │
│ Locals │ Registers │ Globals │ Thread Locals   │              │  ← 0.20
│ Types  │ Procedures (标签在下方)                │              │
├────────────────────────────────────────────────┤              │
│ Call Stack │ Modules  (标签在下方)              │              │
└────────────────────────────────────────────────┴──────────────┘
    0.85                                          0.15
```
窗口属性:**最大化**、显示器 `DISPLAY1`、横向切分(`split_x`)。

> 调乱了 → **F1** → `Reset To Default Panel Layout`。

---

## 16. 附录 C:官方资源

| 资源 | 地址 |
|---|---|
| 代码仓库(现在的正式地址) | <https://github.com/EpicGamesExt/raddebugger> |
| 发布页(下载) | <https://github.com/EpicGamesExt/raddebugger/releases> |
| 老地址(会跳转) | <https://github.com/EpicGames/raddebugger> |
| 厂商项目页 | <https://www.radgametools.com/debug.htm> |
| 报 bug / 提问 | 仓库的 Issues |
| **本地完整手册** | `C:\THOTH\RuanJian\raddebugger-0.9.28-alpha\raddbg_readme.md` ⭐ |

---

## 附:这份文档的事实清单(便于回查)

| 结论 | 依据 |
|---|---|
| 版本 0.9.28-alpha | ✅ 窗口标题 `The RAD Debugger (0.9.28 ALPHA)` |
| 安装路径与文件清单 | ✅ 实际解压目录 |
| 下载完整性 | ✅ SHA256 与 GitHub 官方发布值一致 |
| 界面结构 / 菜单栏 / 标签名 | ✅ 实际启动后截图(modlens 读图) |
| 默认面板布局 | ⚙️ `%APPDATA%\raddbg\default.raddbg_user` |
| 完整默认键位表 | ⚙️ 同上(214 行配置文件) |
| 命令清单 / 参数 / views / 断点语义 / 配置文件规则 | 📖 `raddbg_readme.md`(721 行) |
| `raddbg_markup.h` 的宏 | ✅ 直接读头文件 |
| 我们的 exe / pdb 路径与大小 | ✅ 实际列目录 |
| 中文注释显示异常 | ⚠️ 截图读图结果,**带不确定标记**,未独立复核 |
| `fmt=bgra8` 对我们的内存序是否正确 | 🟡 **推断**,未实测 |
| 带目标启动不会自动运行 | ✅ 实测(启动后进程不存在,要按 F5) |
| **`raddbg.exe` 能调、`raddbg.com` 不能** | ✅ 实测对照(各试多次;对照组 `cdb.exe` 正常) |
| 本机调试功能本身是否正常 | ✅ `cdb.exe` 成功启动并附加了 `ping.exe` 与我们的 exe |

> 文档写完时间:2026-09-21
