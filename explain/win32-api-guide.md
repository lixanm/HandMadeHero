# Win32 API 检索速查指南

> 回答的问题：**"Windows 函数那么多，想实现某个功能时我该用哪个？想实现的功能到底有没有对应函数？"**
>
> 结论先行：**没有人背 API。** 老手靠的是「定位 + 核对」两个动作，而不是记忆。这份文档就是那两个动作的具体操作手册。
>
> 生成环境：Visual Studio 2022 Professional + Windows SDK 10.0.26100.0（本机 `C:\Users\thoth`）

---

## 0. 先看规模：为什么"背下来"这条路线不存在

本机实测（命令见第 8 节）：

```
um 目录头文件个数:        1509
um 中带 WINAPI 的声明行:  18599     ← 仅 C 风格 Win32 导出函数，不含 COM / DirectX / WinRT
winuser.h 里的 WINUSERAPI 声明: 724  ← 仅"窗口与消息"这一块
```

老手常驻记忆的大概 100~300 个。其余全靠查。**这不是水平差距，这是正常分工。**

---

## 1. 唯一真相是头文件，不是文档

同一个坑会以两种面貌出现：

| 现象 | 真实原因 |
|---|---|
| 文档写 `int __clrcall WinMain(...)`，编不过 | 2019 年微软文档迁移时把 Win32 签名块换成了 C++/CLI 风格，**至今未修**（同一页的 Example 用的却是正确的 `APIENTRY`） |
| `MessageBox(0, "文字", "标题", MB_OK)` 报 `C2664` 无法转 `LPCWSTR` | `winuser.h` 里 `#ifdef UNICODE → MessageBoxW`，项目字符集是 Unicode |

**规则：文档用来看「这函数干什么、参数什么含义、要哪个 lib」；签名一律以头文件为准。**

### 现场证据（`winuser.h` 原文）

```c
// winuser.h:4439
WINUSERAPI HWND WINAPI CreateWindowExW(
    _In_ DWORD dwExStyle, _In_opt_ LPCWSTR lpClassName, _In_opt_ LPCWSTR lpWindowName,
    _In_ DWORD dwStyle, _In_ int X, _In_ int Y, _In_ int nWidth, _In_ int nHeight,
    _In_opt_ HWND hWndParent, _In_opt_ HMENU hMenu, _In_opt_ HINSTANCE hInstance,
    _In_opt_ LPVOID lpParam);

// winuser.h:9222
WINUSERAPI int WINAPI MessageBoxW(
    _In_opt_ HWND hWnd, _In_opt_ LPCWSTR lpText, _In_opt_ LPCWSTR lpCaption, _In_ UINT uType);

#ifdef UNICODE
#define MessageBox  MessageBoxW      // ← 泛型名就是在这里被替换掉的
#else
#define MessageBox  MessageBoxA
#endif

// winuser.h:4453
#ifdef UNICODE
#define CreateWindowEx  CreateWindowExW
```

### 实操动作

- **VS 里对任何 API 名字按 `F12`（转到定义）**，直接跳到 `winuser.h`。参数类型看不懂，就对着类型再 `F12`（例如 `LPCWSTR` → 看到它是 `const wchar_t *`）。
- 命令行全文检索（不开 IDE 也能用，"记得有这函数但想不起全名"时最好用）：

```powershell
Select-String -Path "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\*.h" -Pattern "RegisterClass"
```

- 声明块里的 `_In_` / `_In_opt_` / `_Out_` 是 SAL 注解，告诉你参数是否允许 NULL、是输入还是输出，**值得读**（注意是下划线宏写法，不是文档里的 `[in]` 方括号写法——方括号那种是 ATL 属性，本机 C++ 编不过，会报 `C3115`/`C4467`）。

---

## 2. 找函数的四条路径（按你手上已知的信息量排序）

| 你已经知道的 | 做法 |
|---|---|
| **知道属于哪个子系统**（窗口 / 文件 / 音频 / 进程） | 走 [Windows API index](https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-api-list)，它按技术分章节列出全部函数 |
| **只知道"我要实现 X"** | 英文搜 `how to X win32`（中文命中率低）。优先看 Stack Overflow、[Handmade Network 论坛](https://hero.handmade.network/forums)、Raymond Chen 的 *The Old New Thing*。答案里通常点名了 API，拿到名字后回到路径 1 / 第 1 节核对签名 |
| **只记得半个名字** | 头文件搜前缀（`Enum`、`Get`、`Register`…）+ VS 的 IntelliSense（打下 `CreateW`，候选全出来） |
| **完全不知道有没有** | 用第 3 节的命名约定反推函数名，再用第 4 节五道关卡验证 |

---

## 3. 命名约定 = 一份免费索引

Win32 命名高度系统化。学会规则你就能"猜"出函数名，再去头文件验证。

| 线索 | 含义 | 例子 |
|---|---|---|
| `XxxEx` | 扩展版，通常取代旧版 | `CreateWindow` → `CreateWindowEx`；`RegisterClass` → `RegisterClassEx` |
| 尾部 `A` / `W` | ANSI / Unicode 两版，泛型名由 `UNICODE` 宏二选一 | `MessageBoxA` / `MessageBoxW` |
| `Get*` / `Set*` | 读状态 / 写状态 | `GetClientRect` / `SetWindowPos` |
| `Create*` / `Destroy*`、`Open*` / `Close*` | 生命周期成对出现 | `CreateWindowEx` / `DestroyWindow` |
| `Register*` / `Unregister*` | 向系统登记一个类、热键、服务 | `RegisterClassEx` / `RegisterHotKey` |
| `Enum*` | 遍历（配回调或数组） | `EnumDisplayMonitors` / `EnumChildWindows` |
| `Query*` | 查询"可能拿不到"的信息（返回 BOOL + 输出参数） | `QueryPerformanceFrequency` |
| `*Proc` / `*Callback` | 回调函数类型 | `WNDPROC` / `TIMERPROC` |
| `H*` | 句柄，不透明，**不要解引用** | `HWND` / `HDC` / `HINSTANCE` |
| `LP*` | 16 位时代的"长指针"，现在就是普通指针 | `LPSTR` == `char *` |
| `P*` / `*_T` | 指针 / 指向 TCHAR 的指针 | `PWSTR`、`LPTSTR` |
| `WM_*` | 窗口消息 | `WM_CREATE`、`WM_DESTROY`、`WM_QUIT` |
| `WS_*` / `CS_*` / `MB_*` / `SW_*` | 窗口样式 / 类样式 / 消息框标志 / ShowWindow 命令 | `WS_OVERLAPPEDWINDOW`、`CS_HREDRAW` |
| `ERROR_*` / `ID*` | 错误码 / 资源与控件 ID | `ERROR_ALREADY_EXISTS`、`IDI_APPLICATION` |

**反推示例**：「怎么遍历显示器？」→ 猜 `Enum*` + `Monitor` → 搜 `Enum.*Monitor` → 命中 `EnumDisplayMonitors` / `EnumDisplaySettingsW`。

---

## 4. 确认"到底存不存在"：五道关卡，按顺序过

1. **概念层** — API index 里有对应章节吗？没有，说明这事可能不归 Win32 管（见第 5 节）。
2. **编译层** — 头文件里有声明吗？注意 `#if (_WIN32_WINNT >= 0x0600)` 这类条件编译：有的函数要定义了目标版本才可见。
3. **链接层** — 文档 Requirements 表的 **Library** 决定要加哪个 `.lib`。
   VS：项目属性 → 链接器 → 输入 → 附加依赖项。
   `MessageBox` 什么都不用加（`user32.lib` 默认就链）；`PlaySound` 必须加 `winmm.lib`。**这是新手最常见的"编译过了、链接不过"。**
4. **运行层** — 文档的 *Minimum supported client* 决定能不能跑。`dumpbin /exports` 能看这台机器上 DLL 到底导不导出：

   ```
   > dumpbin /exports C:\Windows\System32\user32.dll | findstr /i "RegisterClassExW CreateWindowExW MessageBoxW"
        1628   7B 0001C110 CreateWindowExW
        2161   29E 0005ED90 MessageBoxW
        2242   2F3 0001C0D0 RegisterClassExW
   ```

   对可选 API，还可以运行时 `LoadLibrary` + `GetProcAddress` 探测，做到老系统上优雅降级。
5. **宿主层** — 很多能力**不在 C 风格 Win32 里**：WIC（解码 PNG）、Media Foundation（解码音视频）、WASAPI（音频采集）都是 COM 接口；GPU 绘制在 DXGI / D3D11 / OpenGL。找不到函数是正常的——该换层了。

---

## 5. 什么时候答案是"根本没有这个函数"

Win32 只提供**机制**，不提供**内容**：

| 你想做的 | Win32 里没有 | 该用什么 |
|---|---|---|
| 加载/保存 PNG、JPEG | ✗ | WIC（COM）、GDI+、stb_image |
| 解码 mp3 / 播放音频流 | ✗ | Media Foundation、XAudio2、WASAPI、miniaudio |
| 解析 JSON / XML | ✗ | 自己写或用第三方库 |
| 用 GPU 画东西 | ✗ | DXGI + D3D11/12、OpenGL、Vulkan |
| 读手柄 | ✗（XInput 是独立库） | XInput（`xinput.lib`）、DirectInput |
| 全局热键之外的键鼠原始输入 | 半有 | Raw Input（`GetRawInputData`）、XInput |

遇到这种情况，正确动作不是继续找函数，而是问：**"这一层该由谁负责？"** 然后引对应的库或 COM 接口。Handmade Hero 后半程也是这么换层的（D3D11 + XAudio2）。

---

## 6. 子系统速查表：子系统 → 头文件 → 常用函数

| 子系统 | 头文件 | 需要的 lib | 常用函数 |
|---|---|---|---|
| 窗口 / 消息 | `winuser.h` | user32 | `RegisterClassExW`、`CreateWindowExW`、`ShowWindow`、`GetMessageW`、`TranslateMessage`、`DispatchMessageW`、`DefWindowProcW`、`DestroyWindow`、`PostQuitMessage`、`AdjustWindowRect`、`SetWindowPos`、`GetClientRect`、`MessageBoxW`、`GetSystemMetrics` |
| 2D 绘制 (GDI) | `wingdi.h` | gdi32 | `CreateCompatibleDC`、`CreateDIBSection`、`StretchDIBits`、`BitBlt`、`SelectObject`、`DeleteObject`、`GetStockObject`、`CreateSolidBrush`、`TextOutW` |
| 文件 IO | `fileapi.h` | kernel32 | `CreateFileW`、`ReadFile`、`WriteFile`、`SetFilePointerEx`、`GetFileSizeEx`、`FindFirstFileW` / `FindNextFileW`、`CreateDirectoryW`、`DeleteFileW`、`MoveFileW`、`GetFullPathNameW` |
| 内存 | `memoryapi.h` | kernel32 | `VirtualAlloc`、`VirtualFree`、`HeapAlloc`、`GetProcessHeap`、`MapViewOfFile` |
| 进程 / 线程 / 同步 | `processthreadsapi.h`、`synchapi.h` | kernel32 | `CreateThread`、`CreateProcessW`、`WaitForSingleObject`、`CreateEventW`、`CreateMutexW`、`Sleep`、`GetExitCodeProcess`、`TerminateProcess` |
| 高性能计时 | `profileapi.h`、`sysinfoapi.h` | kernel32 | `QueryPerformanceCounter`、`QueryPerformanceFrequency`、`GetSystemTimeAsFileTime`、`timeGetTime`（winmm） |
| 键盘 / 鼠标 | `winuser.h` | user32 | `GetKeyboardState`、`GetKeyState`、`GetAsyncKeyState`、`GetCursorPos`、`SetCapture`、`ReleaseCapture` + `WM_KEYDOWN` / `WM_CHAR` 等消息 |
| 手柄 | `xinput.h` | xinput | `XInputGetState`、`XInputSetState` |
| 音频输出 | `mmsystem.h`（旧） | winmm | `waveOutOpen`、`waveOutWrite`、`PlaySound`；新项目用 XAudio2（`xaudio2.lib`）或 WASAPI（`audioclient.h`，COM） |
| 网络 | `winsock2.h` | ws2_32 | `WSAStartup`、`socket`、`connect`、`send`、`recv`、`closesocket`；HTTP 可用 WinHTTP（`winhttp.lib`） |
| 注册表 / 配置 | `winreg.h` | advapi32 | `RegOpenKeyExW`、`RegQueryValueExW`、`RegSetValueExW`；简单配置用 `GetPrivateProfileStringW` |
| 字符串 / 编码转换 | `stringapiset.h` | kernel32 | `MultiByteToWideChar`、`WideCharToMultiByte` |
| 错误 / 调试输出 | `errhandlingapi.h`、`debugapi.h` | kernel32 | `GetLastError`、`FormatMessageW`、`OutputDebugStringW`、`IsDebuggerPresent` |
| 显示器 / GPU | `winuser.h`（枚举） | user32 | `EnumDisplayMonitors`、`EnumDisplaySettingsW`、`ChangeDisplaySettingsExW`；真正绘制走 DXGI + D3D11 |

> 提示：Windows 子系统（`/SUBSYSTEM:WINDOWS`）的程序没有控制台，`printf` 看不到东西。调试输出用 `OutputDebugStringW` + DebugView，或者临时切回 Console 子系统。

---

## 7. VS 快捷键 / 工具

| 操作 | 快捷键 |
|---|---|
| 转到定义（跳到头文件看真实签名） | `F12` |
| 全解决方案搜符号 | `Ctrl + T` |
| 搜文件 / 在指定目录搜文本 | `Ctrl + Shift + F` |
| 打开官方文档 | `F1`（**打开后务必用 F12 回头核对签名**） |
| 类视图 / 对象浏览器 | `Ctrl + Shift + C`（查看全部类型成员） |
| IntelliSense 成员列表 | 输入 `.` / `->` / `::` 自动弹出，或 `Ctrl + J` |

离线文档工具：[Zeal](https://zealdocs.org/)（Handmade Network 社区推荐），断网也能查。

编译器 / 链接器工具（在 "x64 Native Tools Command Prompt for VS 2022" 里用）：

```
cl /?                     编译器选项
link /?                   链接器选项
dumpbin /exports x.dll    看 DLL 导出了哪些函数
dumpbin /headers x.exe    看 PE 头（子系统、入口点等）
dumpbin /symbols x.obj    看目标文件符号（排查 LNK2019 神器）
```

---

## 8. 本机可用命令（复制即用）

```powershell
# 在 SDK 头文件里全文检索（把关键字换成你要找的）
Select-String -Path "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\*.h" -Pattern "EnumDisplayMonitors"

# 看某个函数的声明在第几行（例：CreateWindowExW）
Select-String -Path "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\winuser.h" -Pattern "CreateWindowExW"

# 看这台机器上某个 DLL 是否真的导出某函数
dumpbin /exports C:\Windows\System32\user32.dll | findstr /i "CreateWindowExW"

# 命令行编译（VS 开发者环境）
cl /nologo /c /DUNICODE /D_UNICODE yourfile.cpp
```

---

## 9. 错误速查（本项目实际踩过的都在这）

| 错误 | 含义 | 修法 |
|---|---|---|
| `C3193: "__clrcall": 需要"/clr"或"/ZW"` | 抄了文档里 C++/CLI 风格的签名 | 用 `WINAPI` / `APIENTRY` / `CALLBACK`（都是 `__stdcall`） |
| `C3115: "in": 在 "xxx" 上不允许使用此特性` + `C4467 已弃用 ATL 属性` | 把文档的 `[in]` / `[in, optional]` 方括号写法抄进本机 C++ | 本机 C++ 用 SAL 宏：`_In_` / `_In_opt_` / `_Out_`，或者干脆不写 |
| `C2664: 无法将参数 2 从 "const char [9]" 转换为 "LPCWSTR"` | 项目字符集是 Unicode（`/DUNICODE`），窄字面量喂给了 `...W` 版本 | ① 用宽字符串 `L"..."`；② 项目属性 → 高级 → 字符集改成"使用多字节字符集"；③ 显式调 `MessageBoxA` |
| `C2373: "WinMain": 重定义；不同的类型修饰符` | 自己声明的 `WinMain` 与 `winbase.h` 里的 `WINAPI` 版本冲突 | 加 `WINAPI`，别重复声明 |
| `LNK2019: 无法解析的外部符号 main` | 链接子系统是 Console（`/SUBSYSTEM:CONSOLE`），CRT 入口找 `main`，你只提供了 `WinMain` | 链接器 → 系统 → 子系统改成 Windows；同时去掉 `_CONSOLE` 宏 |
| `LNK2019: 无法解析的外部符号 _WinMain@16` | x86 下 `WinMain` 没加 `__stdcall`（`WINAPI`） | 加 `WINAPI`；x64 下调用约定被忽略，不会出这个错 |
| `LNK2019: 无法解析的外部符号 XXX`（某个 API） | 少了提供它的 `.lib` | 链接器 → 输入 → 附加依赖项加上第 6 节表里的 lib |
| `LNK1120: N 个无法解析的外部命令` | 上面那些 LNK2019 的汇总计数 | 先修 LNK2019，这个会一起消失 |
| 程序跑起来什么都没发生 | Windows 子系统程序没有控制台输出 | 用 `OutputDebugStringW` + DebugView，或临时切回 Console |

---

## 10. 我的 API 笔记（自己往下加）

| 日期 | API | 一句话作用 | 需要哪个 lib | 备注 |
|---|---|---|---|---|
|  | `MessageBoxW` | 弹一个模态消息框 | user32（默认链） | 泛型名 `MessageBox` 由 `UNICODE` 宏二选一 |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |

---

## 一句话心法

> **文档看用途，头文件看签名，编译器做裁判。**
>
> 需求 → 想清楚属于哪个子系统 → 猜一个有命名规律的名字（`Enum*` / `Create*Ex` / `Get*`）→ 头文件里搜出来核对 → 写最小测试程序验证。
>
> 查文档不是菜，是工序。Casey 在 Handmade Hero 里也现场翻 MSDN，甚至专门有一集讲"怎么读 MSDN"。
