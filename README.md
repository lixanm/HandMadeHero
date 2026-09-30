# HandMade — 跟着 Handmade Hero 从零写 Win32 平台层（**目标是类银河城游戏**）

这个仓库是我跟着 [Handmade Hero](https://handmadehero.org/)（Casey Muratori）一行一行写下来的代码。
目标是**不用任何框架、不用任何引擎**，把 Windows 平台层和游戏引擎全部自己写出来。

> **跟的是什么、不跟什么**（2026-09-24 补）：
> 我参考的是 **Handmade Hero 的平台层与引擎层** —— 窗口 / 输入 / 音频 / 文件 / 线程 / 计时 / 资产加载 / 内存，
> 以及**和游戏类型无关**的那些部分（渲染器、音频、资产、工具）。
> **不照搬它这款游戏**：手工英雄最后做出来的是一个**地牢探索类**游戏（程序化房间 + 真 3D 投影的 2.5D 场景 + 光照、Brains 那一套）；
> **我要做的是类银河城（Metroidvania）**。
> 所以：**引擎层跟着学**；**玩法、关卡与地图结构、能力解锁与回溯设计，按类银河城自己来做**。

> **AI 可以直接写代码**(2026-09-30 起):原来「代码必须手敲、AI 默认只读」的约束已取消(见 [`.agents/adr/0007`](.agents/adr/0007-ai-writes-code.md));保留的是「**说清楚 + 留退路**」(`NOTES.md` §1)。学习目标不变,只是打字不再由一个人扛。
> 文件是 `.cpp`，但**按 C 风格写**（Casey 的写法）：不用类、不用 STL、不用异常和 RTTI（`-GR- -EHa-` 就是把它们关掉）。

---

## 当前进度

| 阶段 | 内容 | 状态 |
|---|---|---|
| Day 1 | 建工程、搞清楚字符集/子系统/入口点 | ✅ 完成 |
| Day 2 | 窗口四步 + 消息循环 + `WM_PAINT` | ✅ 完成 |
| Day 3 | 后备缓冲：`CreateDIBSection` → 改成**自己 `VirtualAlloc`** | ✅ 完成 |
| Day 4 | 往缓冲里画像素、每帧贴图、`biHeight` 取负、**画面动起来** | ✅ 完成 |
| Day 5 | 全局量收进结构体、**固定 1280×720**、`Win32GetWindowDimension` | ✅ 完成 |
| Day 6 | 手柄（XInput 动态加载）+ 键盘输入 | 🚧 **进行中** |

> 📌 **Day 编号 = "我跟视频学到哪一集"，不是我们游戏的进度条。**
> 平台层那一段大约到 **025** 就结束；之后引擎层继续参考他的做法，但**游戏本身走类银河城那条线**。

- 最新提交：`9c56e6e`（2026-09-21），已同步到 GitHub → `git rev-parse HEAD origin/main` 两边一致。
- **Day 6 进度**：手柄的**动态加载三件套**已打通（宏生成签名 + stub + 函数指针 + `#define` 重定向、`Win32LoadXinput()`）；
  **键盘也已接上并实测通过**（`WM_KEYDOWN`/`WM_KEYUP` 共用一个块，用 `LPARAM` 的 bit30/31 过滤按键重复）。
  `build.bat` 现在是 **`[build] OK`**（0 error 0 warning）。
- **还差两处收尾**：两个 stub 仍然 `return 0`（那是 `ERROR_SUCCESS`，语义应该是 `ERROR_DEVICE_NOT_CONNECTED`）；
  `Win32LoadXinput()` 缺"取不到导出函数就退回 stub"的兜底。

---

## 现在这个程序能做什么

- 打开一个标题为 `Handmade Hero` 的窗口；点右上角 X **能正常退出**。
- **自己分配**一块后备缓冲（`VirtualAlloc`，1280×720×4 = 3,686,400 字节）+ 一个 `BITMAPINFO` 说明书，
  内存归我们自己，Windows 只负责显示（不需要 DC、不需要位图句柄）。
- 每帧往缓冲里写"怪渐变"：蓝跟着 X、绿跟着 Y，用**一次 32 位写**代替四次字节写；偏移量每帧 +1 → 画面滚动。
- 用 `StretchDIBits` 把整块缓冲贴到客户区，`biHeight` 取负让它自上而下。
- 渲染分辨率**固定 1280×720**，窗口拉伸不再重建缓冲（画面会拉伸，宽高比校正还没做）。
- **读键盘和手柄**：键盘走窗口消息（W/A/S/D/Q/E、方向键、空格、Esc，已实测），手柄走每帧轮询
  （`xinput1_3.dll` 是运行时 `LoadLibraryA` 进来的，**故意不链** `xinput.lib`）。

**还没做**（按 Day 顺序）：

- 计时 / 帧率控制 —— 现在是 busy loop，会**占满一个 CPU 核**；
- Day 6 那两处收尾（见上）；
- **真正的游戏层** —— 现在平台层直接往缓冲里画渐变，还没有"游戏代码"这一层。
  这一层以后**按类银河城的设计走**（地图连通、能力解锁与回溯、存档点、Boss），**不照搬** HH 的地牢那一套。

---

## 目录结构

```
HandMade.sln                        解决方案（VS2022）
NOTES.md                            工作约定：AI 的行为分档（直接做 / 先说再做 / 必须等批准）
GLOSSARY.md                         词汇表（只放最容易各写各的那几个词；完整译名以术语表为准）
.dsh/skills/                        给 AI 的技能（会自动加载：盘问 / 调试循环 / 写文档判据）
.agents/adr/                        关键决策记录（决定了什么、为什么、代价是什么）
HandMade/                           项目文件夹
├── HandMade.vcxproj                ★ 编译/链接设置的唯一真源（VS2022 里按 F7 就是它）
├── code/
│   └── win32_handmade.cpp          目前唯一的源文件（Win32 平台层）
├── build.bat                       命令行一行构建（只是 msbuild 的包装，不含编译选项）
└── shell.bat                       打开一个带 VS 开发环境的命令行
debug.bat                           先构建，再用 RAD Debugger 打开产物（在仓库根目录）
compile_commands.json               给 clangd / Sourcetrail 用的编译数据库（clang 语法）
HandMade.srctrlprj                  Sourcetrail 工程文件
explain/                            学习文档
├── !HandmadeHeroExplain/           按集的视频讲解笔记 + 术语表
├── 2026-9/                         按天的工作日志
└── 手工英雄访谈/                    Casey 其它访谈/讲座的笔记（3 类 32 集，各带字幕原文）
```

> `build/`、`Debug/`、`Release/`、`x64/`、`explain/!subtitle/` 已被 `.gitignore` 忽略；
> Sourcetrail 的索引库 `*.srctrldb` / 书签 `*.srctrlbm` 同样忽略（本机生成、每次重建都变）。

---

## 怎么编译运行

### 方式 A：Visual Studio 2022（**唯一真源**，2026-09-30 起）

打开 `HandMade.sln`，选 `Debug|x64` → **F7** 生成、**F5** 调试、`Ctrl+F5` 不调试运行。

- **编译/链接设置的唯一真源 = `HandMade\HandMade.vcxproj`。**
  以前这里有两套配置：命令行的 `build.bat` 和 VS 工程，而两者的选项并不一致（警告等级、`/WX`、预处理宏、运行库、链接库，连 exe 名字都不同），
  "VS 里能过、命令行过不了"这类问题迟早会出现。现在统一：**VS 工程说了算**。
- 输出：`x64\Debug\HandMade.exe` / `x64\Release\HandMade.exe`（在**仓库根目录**下，不在 `HandMade\` 里）
- 四个配置都编过：`Debug|Win32`、`Release|Win32`、`Debug|x64`、`Release|x64`（2026-09-30 用 msbuild 全量 Rebuild 复核）

想在命令行 / 脚本里构建：

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" `
  HandMade.sln /p:Configuration=Debug /p:Platform=x64
```

### 方式 B：`build.bat`（只是上面那条 msbuild 的包装）

```cmd
HandMade\build.bat            :: Debug|x64（默认）
HandMade\build.bat release    :: Release|x64
```

它**本身不含任何编译选项**（所以不可能和 VS 工程跑偏），只做两件事：调 `VsDevCmd.bat` 让 `msbuild` 可用，然后 `msbuild HandMade.sln`。

### 方式 C：编译 + 进调试器

```cmd
HandMade\debug.bat
```

先调 `build.bat`，再用 [RAD Debugger](https://github.com/EpicGames/raddebugger) 打开刚编出来的 exe。
装在本机 `C:\THOTH\RuanJian\raddebugger-0.9.28-alpha\`；**必须用 `raddbg.exe`**——
那个 `raddbg.com` 控制台外壳会**丢掉命令行上的目标**，表现是"能启动、但什么都调试不到"。
完整用法见 `explain/RAD Debugger 使用手册 (0.9.28-alpha).md`。

### 方式 D：只读代码 / 逛调用图

用 Sourcetrail 打开 `HandMade.srctrlprj`。它走 clang，**不认 MSVC 那串参数**，
所以另外准备了一份 clang 语法的 `compile_commands.json`。用法见 `explain/Sourcetrail 使用指南.md`。

---

## 关键工程设置

| 设置 | 值 | 为什么是这样 |
|---|---|---|
| **字符集** | **使用多字节字符集（MultiByte）** | 和视频里的 ANSI 风格一致：字符串直接写 `"..."`，不需要 `L"..."` |
| **子系统** | **Windows** | 入口点是 `WinMain`（写成 Console 会报 `LNK2019: 无法解析的外部符号 main`） |
| 平台工具集 | v143（VS2022） | |
| 配置 | Debug / Release × Win32 / x64 | |
| 警告等级 | **`/W4` + 警告当错 `/WX`** | 对齐 Casey：出现**任何**警告都算编译失败 |
| 抑制的警告 | `4201 4100 4505 4189 4457 4456 4819` | 和 Casey 同一份名单。⚠️ 它同时把"定义了没用到的局部变量"这类问题压掉了，编译器不会替你抓 |
| 运行库 | **`/MTd`（Debug）/ `/MT`（Release）** | **静态链接 C 运行库**，exe 不依赖 VC 运行库 DLL（2026-09-30 用 `dumpbin /dependents` 核实：只有 `USER32/GDI32/KERNEL32`） |
| 优化 | `/Od`（Debug）/ `/O2`（Release） | |
| RTTI / 异常 | `/GR-` `/EHa-` | 我们按 C 风格写，这两个都不用 |
| 全路径诊断 | `/FC` | 报错带完整路径；VS 里双击错误 / 错误列表能直接跳到那一行 |
| 预处理宏 | `HANDMADE_SLOW` / `HANDMADE_INTERNAL` / `HANDMADE_WIN32` | Casey 的三个开关：Debug 全为 1，Release 前两个为 0（目前代码还没用到，先按他的约定定义好） |
| 输出 | `x64\<Config>\HandMade.exe` | MSBuild 的解决方案级输出路径 |

**链接的库**：`user32.lib gdi32.lib winmm.lib` —— 三者都由工程显式链接（GDI 负责画窗口；winmm 以后计时/音频要用）。

> ⚠️ VS 工程另外还开着 **`/permissive-`**（比 Casey 的参数更严格）。目前代码能过；哪天它报了 Casey 那边不报的错，那是**严格度**差异而不是代码错，可以把「符合模式」关掉来对齐。

> ⚠️ **手柄不链接 `xinput.lib`**（Day 006 的结论，和"以后要用就加库"的直觉相反）：
> 一旦链上，Windows 在**启动时**就要求 `xinput1_3.dll` 存在，没装的机器**游戏整个打不开**；
> 所以改成运行时 `LoadLibraryA("xinput1_3.dll")` + `GetProcAddress`，把地址存进函数指针
> （初值指向一个 stub）。以后用到 D3D 之类才需要往链接列表里加库。

---

## 需要的环境

- Windows 10 / 11
- **Visual Studio 2022**，勾选"使用 C++ 的桌面开发"工作负载（本机是 Professional + Windows SDK 10.0.26100）
- 可选：
  - [RAD Debugger](https://github.com/EpicGames/raddebugger) 0.9.28-alpha（见方式 C）
  - [Sourcetrail](https://github.com/CoatiSoftware/Sourcetrail) 2021.4（**已停止维护**，见方式 D）

---

## 学习文档（`explain/`）

### 按集的视频讲解（`!HandmadeHeroExplain/`）

| 文件 | 内容 |
|---|---|
| `Day 001 - Day 004 术语表.md` | **译名以它为准**（后备缓冲 / 客户区 / 脏区域 / 离屏缓冲……） |
| `Day 003 - Allocating a Backbuffer.md` | `CreateDIBSection` / `BITMAPINFO` 逐字段 / `StretchDIBits` / 内存管理 |
| `Day 004 - Animating the Backbuffer.md` | 画像素、`BB GG RR xx` 字节序、一次写 32 位、`biHeight` 为什么取负（含实测耗时） |
| `Day 005 - Windows Graphics Review.md` | 全局量收进结构体、固定分辨率、按值传 vs 传指针、脏区域 |
| `Day 006 - Gamepad and Keyboard Input.md` | XInput 轮询、**为什么不链 `xinput.lib`**、宏/stub/函数指针、键盘 `LPARAM` 解密 |

### 按天的工作日志（`2026-9/`）

| 文件 | 内容 |
|---|---|
| `2026-9-18.md` | 把"编不过"变成"窗口能画"的全过程（字符集、子系统、`WNDCLASS` 逐字段、消息循环） |
| `2026-9-19.md` | 看完 Day 2 Q&A 之后，按 Casey 的建议让窗口能正常关闭 |
| `2026-9-20.md` | Day 4 代码落地 + "一次写 32 位" + 编辑器覆盖事故的复盘 |
| `2026-9-21.md` | Day 5 收尾进仓库、Day 006 讲解、两条工具链、第一次 push 上 GitHub |

### 工具与通用资料

| 文件 | 内容 |
|---|---|
| `HANDMADE_HERO_终点与导航.md` | **《手工英雄》这款游戏的地图**（终点 Day 667，项目已退休）：哪一段在做引擎、哪一段在做**他自己的**游戏内容。<br>⚠️ 这是**参考**，不是我们的设计文档 —— 我们做的是**类银河城**，只借用它的**引擎层时间线**（尤其平台层 001–025）。<br>🟡 该文档自己也声明过：里面关于视频内容的描述是**推断**（作者没看过全部视频） |
| `win32-api-guide.md` | **Win32 API 检索速查**："想实现某功能该用哪个函数"的操作手册 |
| `Sourcetrail 使用指南.md` | 装、配、建索引、看调用图（含 MSVC 参数为什么不认） |
| `RAD Debugger 使用手册 (0.9.28-alpha).md` | 界面、断点、监视、`--ipc`、常见坑 |
| `手工英雄访谈/` | Casey 的其它访谈与讲座笔记：**人工智能相关** 10 集、**软件质量**（Refterm 讲座等）12 集、**注重性能的编程方式** 10 集；每集都配了字幕原文 |

---

## 参考资料

- [Handmade Hero 官网](https://handmadehero.org/) ｜ [handmade.network 论坛](https://hero.handmade.network/forums)
- [Windows API index（官方文档）](https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-api-list)
- 查 API 的心得：**文档看用途，头文件看签名** —— 在 VS 里对任何 API 名字按 **F12** 跳到 `winuser.h`/`wingdi.h`，那才是权威
  （微软文档出过错，比如 `WinMain` 的签名块至今写着 C++/CLI 的 `__clrcall`）
- 本仓库里的 AI 工作约定见 [`NOTES.md`](NOTES.md)：代码可以直接改（先说一句）、不擅自提交、不知道就说不知道
