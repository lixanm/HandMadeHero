# HandMade — 跟着 Handmade Hero 从零写 Win32 平台层

这个仓库是我跟着 [Handmade Hero](https://handmadehero.org/)（Casey Muratori）一行一行写下来的代码。
目标是**不用任何框架、不用任何引擎**，把 Windows 平台层和游戏引擎全部自己写出来。

> 目前是一个学习项目，代码还很原始：一个文件、一个窗口、一块还没开始用的后备缓冲。

---

## 当前进度

| 阶段 | 内容 | 状态 |
|---|---|---|
| Day 1 | 建工程、搞清楚字符集/子系统/入口点 | ✅ 完成 |
| Day 2 | 窗口四步 + 消息循环 + `WM_PAINT` | ✅ 完成 |
| Day 3 | 后备缓冲（`CreateDIBSection` / `StretchDIBits`） | 📖 讲解已看，代码待写 |

**现在这个程序能做什么**：打开一个标题为 `Handmade Hero` 的窗口；在 `WM_PAINT` 里用 `PatBlt` 在黑白之间交替涂色（Casey 管它叫"癫痫模拟器"）；拖动窗口大小、最小化还原、被遮挡再露出时会重新绘制；点右上角 X **能正常退出**。

**还没做**：往像素里画东西 —— 这正是 Day 3/4 要做的事。

---

## 目录结构

```
HandMade.sln                    解决方案（VS2022）
HandMade/                       项目文件夹
├── HandMade.vcxproj            工程设置都在这里
├── code/
│   └── win32_handmade.cpp      目前唯一的源文件（Win32 平台层）
├── build.bat                   命令行构建（Casey 的工作流，输出到 build/）
├── shell.bat                   打开一个带 VS 开发环境的命令行
└── emacs.bat                   在带 VS 开发环境的上下文里启动 Emacs
explain/                        学习文档
├── 2026-9/                     按天的工作日志
└── .HandmadeHeroExplain/       按集的视频讲解笔记
```

---

## 怎么编译运行

### 方式 A：Visual Studio 2022（入门推荐）

打开 `HandMade.sln`，按 **F5** 运行（`Ctrl+F5` 不调试运行）。

四个配置都能编过：`Debug|Win32`、`Release|Win32`、`Debug|x64`、`Release|x64`。

命令行也行：

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" `
  HandMade.sln /p:Configuration=Debug /p:Platform=x64
```

### 方式 B：`build.bat`（Casey 的工作流）

```cmd
HandMade\build.bat
```

- 输出：`HandMade\build\win32_handmade.exe`
- 脚本里自己调用 VS2022 的 `VsDevCmd.bat`，所以**在哪儿双击都能用**
- 编译参数：`-Od -W4 -WX -MTd -GR- -EHa- -Zi`，链接 `user32.lib gdi32.lib winmm.lib`
  - **`-W4 -WX`：警告等级 4，而且警告当错误** —— 代码里出现任何警告都会编译失败（这是 Casey 故意的）
  - **`-MTd`：静态链接调试版 C 运行库** —— 所以编出来的 exe 只依赖 `KERNEL32/USER32/GDI32`，**拷到别的电脑也能跑**（VS 工程默认的 `/MDd` 则需要目标机器装 VC 运行库）
- 在 Emacs 里按 `M-m` 就会调用它（见 `explain/` 里的键位说明）

---

## 关键工程设置

| 设置 | 值 | 为什么是这样 |
|---|---|---|
| **字符集** | **使用多字节字符集（MultiByte）** | 和视频里的 ANSI 风格一致：字符串直接写 `"..."`，不需要 `L"..."` |
| **子系统** | **Windows** | 入口点是 `WinMain`（写成 Console 会报 `LNK2019: 无法解析的外部符号 main`） |
| 平台工具集 | v143（VS2022） | |
| 配置 | Debug / Release × Win32 / x64 | 四个都能编过 |
| 警告等级 | `/W3`（VS 工程）／`-W4 -WX`（build.bat） | 两条路线的严格程度不一样 |

**不需要手动添加任何 `.lib`**：`user32.lib` / `gdi32.lib` / `kernel32.lib` 由 VS 默认链接（或 `build.bat` 里显式写出）。
以后用到音频（`winmm.lib`）、手柄（`xinput.lib`）、D3D（`d3d11.lib` + `dxgi.lib`）才需要自己加。

---

## 需要的环境

- Windows 10 / 11
- **Visual Studio 2022**，勾选"使用 C++ 的桌面开发"工作负载
- 可选：[Emacs](https://www.gnu.org/software/emacs/)（Casey 用的编辑器，`emacs.bat` 会在带 VS 环境的上下文里启动它，这样 `M-x compile` 和 clangd 都能找到 `cl.exe`）

---

## 学习文档（`explain/`）

| 文件 | 内容 |
|---|---|
| `explain/2026-9/2026-9-18.md` | 工作日志：把"编不过"变成"窗口能画"的全过程（字符集、子系统、`WNDCLASS` 逐字段、消息循环、踩过的坑速查） |
| `explain/2026-9/2026-9-19.md` | 工作日志：看完 Day 2 Q&A 之后，按 Casey 的建议让窗口能正常关闭 |
| `explain/.HandmadeHeroExplain/Day 003 - Allocating a Backbuffer.md` | Day 3 视频讲解笔记：`CreateDIBSection` / `BITMAPINFO` 逐字段 / `StretchDIBits` / 内存管理 |

---

## 参考资料

- [Handmade Hero 官网](https://handmadehero.org/) ｜ [handmade.network 论坛](https://hero.handmade.network/forums)
- [Windows API index（官方文档）](https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-api-list)
- 查 API 的心得：**文档看用途，头文件看签名** —— 在 VS 里对任何 API 名字按 **F12** 跳到 `winuser.h`/`wingdi.h`，那才是权威（微软文档出过错，比如 `WinMain` 的签名块至今写着 C++/CLI 的 `__clrcall`）
