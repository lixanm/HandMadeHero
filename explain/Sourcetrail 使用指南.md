# Sourcetrail 使用指南(完整版)

> **这份文档是什么**:Sourcetrail 的**完整使用说明** —— 从打开项目、索引、界面操作、全部设置项、编辑器插件,到命令行与文件格式;末尾附**本仓库(HandMade)的专用配置**。
>
> **本文件**:`D:\HandMade\explain\Sourcetrail 使用指南.md`
> **版本**:Sourcetrail **2021.4**(这是**最后一个版本**,项目已停止开发)
> **安装位置**(✅ 实测):`C:\Program Files\Sourcetrail\`(`Sourcetrail.exe` + `sourcetrail_indexer.exe`)
> **数据目录**(✅ 实测):`C:\Users\thoth\AppData\Local\Coati Software\Sourcetrail 64-bit\`
>
> **事实来源**:
> - ✅ **实测** —— 在本机上真的跑过 / 真的读过那个文件
> - 📖 **官方文档** —— 仓库根目录的 `DOCUMENTATION.md`(1,684 行,对应 2021.4 版)
> - ⚙️ **本机配置** —— `AppData\...\Sourcetrail 64-bit\ApplicationSettings.xml`
> - 🟡 **推断** —— 没验证
>
> ⚠️ **先说清楚这个工具的现状**:Coati Software 已于 2021 年**停止开发**,GitHub 仓库**已归档**。它现在**能正常用**,但**不会再有更新或修复**。官方文档网站也早已下线,所以本文把官方手册的内容整理进来了。

---

## 0. 一页速查

| 我想…… | 怎么做 |
|---|---|
| **打开 HandMade 项目** | 双击 `D:\HandMade\HandMade.srctrlprj`(**已经配好了**) |
| 索引 / 重新索引 | 菜单 **Edit → Refresh**(`F5`);全量重建 **Edit → Full Refresh**(`Shift+F5`) |
| 找符号 | **Ctrl+F**(模糊匹配,可以跳字母) |
| 全文搜索 | 搜索框里以 **`?`** 开头(不分大小写);**`??`** 区分大小写 |
| 看某个符号被谁用 / 用了谁 | 在 **Graph** 里点它,或 **Ctrl+U** 开 Custom Trail |
| 从图跳到代码 | 点节点 / 点边 → Code 视图显示位置 |
| 在代码里前后跳引用 | **Ctrl+G** / **Ctrl+Shift+G** |
| 从 Sourcetrail 跳到编辑器 | 在 Code 视图里 **按住 Ctrl 点一行**(需装编辑器插件) |
| 收藏一个符号 | **Ctrl+S**(书签),**Ctrl+B** 管理书签 |
| 改字体大小 | **Ctrl + +** / **Ctrl + -** / **Ctrl + 0** |
| 找不到东西了 | **Ctrl+Home**(回到 overview) |

---

## 1. 它是什么(以及**不是什么**)

**是**:一个**交互式源码浏览器**。它把你的代码**索引**成一张符号关系数据库,然后用三个联动视图让你看清结构:

| 视图 | 作用 |
|---|---|
| **Search** | 搜索索引过的符号(模糊匹配 + 自动补全) |
| **Graph** | 以当前符号为中心,画出**所有出边和入边**的依赖关系图 |
| **Code** | 列出当前符号在源码里的**所有出现位置**(代码片段) |

**支持的语言**:
- **C / C++** —— 由 **Clang 11.0.0** 驱动 📖
- **Java**(12 及以下)—— 由 Eclipse JDT 驱动 📖
- **Python 2 / 3** —— 由 Coati 自研的 `SourcetrailPythonIndexer` 驱动 📖

**不是**:
- ❌ **不是编辑器** —— Code 视图**不能改代码**(只读)
- ❌ **不是"打开文件夹就行"** —— 必须先**配置 Source Group 再索引**,而配置方式取决于你的构建系统
- ❌ **不是 IDE 替代品** —— 它是"看结构"的辅助工具
- ❌ **不是编译器** —— 索引用的是**独立的一次 clang 解析**(见 §4.1 的坑)

---

## 2. 安装与文件布局

### 2.1 本机的实际位置(✅ 实测)

| 东西 | 路径 |
|---|---|
| 程序 | `C:\Program Files\Sourcetrail\Sourcetrail.exe`(92 MB) |
| 索引器 | `C:\Program Files\Sourcetrail\sourcetrail_indexer.exe`(91 MB,被 GUI 调用) |
| 静态数据 | `C:\Program Files\Sourcetrail\data\`(字体、配色方案、GUI 资源) |
| 动态数据 / 设置 | `C:\Users\thoth\AppData\Local\Coati Software\Sourcetrail 64-bit\` |
| └ 全局设置 | `ApplicationSettings.xml` |
| └ 窗口布局 | `window_settings.ini` |
| └ 日志 | `log\`(需先在偏好设置里打开 Logging) |
| └ 自带示例工程 | `projects\`(`tutorial` / `tictactoe_cpp` / `tictactoe_py` / `javaparser`) |
| 开始菜单 | `Sourcetrail 64-bit` |

> 📖 官方给的位置是 `C:/Users/You/AppData/Local/Coati Software/Sourcetrail`;本机装的是 **64 位版**,所以多一个 ` 64-bit` 后缀。

### 2.2 Sourcetrail 只用三种文件

| 文件 | 是什么 | 要不要进版本管理 |
|---|---|---|
| **`.srctrlprj`** | **工程文件**(XML,`<version>8</version>`),记录 Source Group / 语言 / 路径 | ✅ **建议提交**(很小,是配置) |
| **`.srctrldb`** | **索引数据库** —— 📖 官方原话:「这实际上就是一个 **SQLite 数据库文件**」 | ❌ **不要提交**(可重建) |
| **`.srctrlbm`** | **书签文件**(XML) | 🟡 看你 —— 想保留书签就提交 |

> 📖 官方还提到:`.srctrldb` 可以用 [DB Browser for SQLite](https://sqlitebrowser.org/) 打开查看。

---

## 3. 上手:打开本仓库(★ 已经给你配好)

### 3.1 直接双击打开

```
双击  D:\HandMade\HandMade.srctrlprj
```

相关的两个文件**已生成并验证**:

| 文件 | 作用 | 验证结果 |
|---|---|---|
| `D:\HandMade\compile_commands.json` | **编译数据库** —— 告诉 Sourcetrail 每个文件"是怎么编译的"(头文件路径、宏) | ✅ 用 `clangd --check` 实测:成功加载、`windows.h` 全部解析、**零语法/语义错误** |
| `D:\HandMade\HandMade.srctrlprj` | 工程文件(类型 = `C/C++ from Compilation Database`,相对指向上面那份) | ✅ 索引实测成功,生成了 `HandMade.srctrldb` |

**索引结果的证据**(✅ 实测):`HandMade.srctrldb` 是合法 SQLite(218 KB),里面能查到我们的符号:

| 符号 | 在索引库里出现的次数 |
|---|---|
| `win32_handmade.cpp` | 118 |
| `win32_offscreen_buffer` | 47 |
| `GlobalBackBuffer` | 7 |
| `RenderWeirdGradient` | 4 |
| `Win32ResizeDIBSection` | 4 |
| `WindowMainWindowCallback` | 4 |

### 3.2 索引(第一次 / 改了代码之后)

打开项目后点 **Start** 开始索引;之后**改了代码**就:

| 操作 | 快捷键 | 效果 |
|---|---|---|
| **Refresh** | **F5** | 只重新索引**改动过的文件**及其依赖 |
| **Full Refresh** | **Shift+F5** | **全量重建**(删旧索引重来) |

📖 **索引对话框里有三种模式**:
- **Updated files** —— 只处理改动过的 + 依赖它们的 + 新文件
- **Incomplete & updated files** —— 顺便重试**上次报错的文件**
- **All files** —— 全部重来

⚠️ **一个坑**(📖 官方原话):如果文件**内容没变**,即使它上次索引出错,Sourcetrail 也**不会重新索引**它 —— 这时要用 **Edit → Force Refresh**(即 Full Refresh)。

### 3.3 怎么知道索引成功

- **状态栏**右下角会显示索引进度条 / 错误计数;
- 索引完成后**自动显示 overview**:Graph 里是全部符号概览,Code 里是统计数字;
- **有错误**就看 **状态视图的 Errors 标签**(见 §5.5)。

### 3.4 命令行索引(不开 GUI)✅ 实测

`Sourcetrail.exe` 支持命令行(这些参数是**从 exe 里提取**的,官方手册**没有**写):

```cmd
:: 索引一个工程(不带 full 时只处理新增/改动的文件)
"C:\Program Files\Sourcetrail\Sourcetrail.exe" --index "D:\HandMade\HandMade.srctrlprj"
```

| 参数 | 含义(exe 内的原文) |
|---|---|
| `index` | **Index a certain project.** + `Project file to index (.srctrlprj)` |
| `full` / `f` | **Index full project** (omit to only index new/changed files) |
| `incomplete` / `i` | **Also reindex incomplete files** (files with errors) |
| `shallow` / `s` | **Build a shallow index** is supported by the project |
| `config` / `config-system-dir` / `config-user-dir` | 指定配置文件位置 |
| `logging-enabled` / `verbose-indexer-logging-enabled` | 打开日志 |
| `indexer-threads` / `use-processes` | 索引线程数 / 多进程索引 |

> 💡 **用途**:把"编译 → 索引"接进脚本里,不用每次手点。
> ⚠️ 它是 GUI 程序,命令行索引时**不会把进度打到控制台**;判断完成看 `.srctrldb` 是否在生成/增长。

---

## 4. 项目设置(Project Setup)全部类型

📖 一个 Sourcetrail 工程由若干 **Source Group(源组)** 组成,**多数项目只需要一个**。工程设置入口:
- 新建:**Start Window → New Project**,或菜单 **Project → New Project**
- 修改:**Project → Edit Project**

工程设置窗口里的操作:改工程名/位置、`Add Source Group`、`+`/`-`/`☐`(新增/删除/复制源组)、**每个源组可以设 `active` 开关**(关掉的源组不参与索引)。

### 4.1 C/C++ from Compilation Database ⭐(我们在用的)

| 设置项 | 说明 |
|---|---|
| **Compilation Database** | 选 `compile_commands.json`。Sourcetrail 用里面的编译命令(含**所有 include 路径和编译开关**)来索引,**每次 Refresh 都会重新读这个文件**,所以数据库变了它就跟上 |
| **Header Files & Directories to Index** | 数据库只说了"哪些源文件属于项目",**头文件要不要索引得你指定**。跳过系统头/第三方头能**大幅提速**。💡 官方提示:直接填**项目根目录**就会索引所有遇到的头文件 |
| **Excluded Files & Directories** | 排除清单(通配符见 §4.8) |
| **Additional Include Paths** | 额外的 `-I` 路径 |
| **Global Include Paths** | 所有工程共用的系统头路径(相当于 `-isystem`) |
| **Additional Compiler Flags** | 额外开关,**要带横线**,比如 `-DRELEASE` |
| **Precompiled Header File / Flags** | 预编译头(可选;不给路径就不生成) |

**我们对这个项目的实际配置**(`HandMade.srctrlprj` 内容):

```xml
<build_file_path>
    <compilation_db_path>compile_commands.json</compilation_db_path>   <!-- 相对工程文件所在目录 -->
</build_file_path>
<indexed_header_paths>
    <indexed_header_path>HandMade/code</indexed_header_path>           <!-- 只索引我们自己的头 -->
</indexed_header_paths>
<name>C/C++ from Compilation Database</name>
<pch_flags><use_compiler_flags>1</use_compiler_flags></pch_flags>
<type>C/C++ from Compilation Database</type>
```

> ⚠️ **坑(重要)**:Sourcetrail 内部用的是 **clang/libclang**(安装目录里有 `license_clang.txt`),所以 `compile_commands.json` 里的命令**必须是 clang 能懂的语法** —— **不能直接照抄 `build.bat` 里的 MSVC 参数**(`-W4 -WX -MTd -Oi -GR- -wd4201 …` 这些 clang 不认)。
> 我们那份数据库里用的是:
> `clang --target=x86_64-pc-windows-msvc -std=c++17 -fsyntax-only -D…` + **9 条 `-isystem` 头文件路径**(从 `VsDevCmd.bat` 的 `%INCLUDE%` 取出的真实路径)。

### 4.2 Empty C/C++ Source Group

不用构建系统时的兜底方案。要填:语言标准、交叉编译目标、**要索引的文件/目录**、排除、源文件扩展名、**Include Paths**、Global Include Paths、macOS framework 路径、**Compiler Flags**、预编译头。

### 4.3 C/C++ Source Group from Visual Studio

📖 流程:点 **Create CDB** → 通知 **Visual Studio 插件**导出编译数据库 → 后面的步骤和 §4.1 一样。
🟡 这条路径依赖 **VS 插件**(官方说支持到 VS2017),我们用的是 VS2022,**本机没验证过**;我们的 `.sln` 建议走 §4.1。

### 4.4 C/C++ Source Group from Code::Blocks

有 `.cbp` 文件就能直接索引(设置项:C/C++ 标准、Header Files to Index、排除、扩展名、额外 include/flag)。
💡 官方提到:**Qt Creator 导入 CMakeLists 时会生成 `.cbp`**。

### 4.5 Java

| 类型 | 关键设置 |
|---|---|
| **Empty Java** | 标准、要索引的目录、排除、扩展名(`.java`)、**Class Path**(依赖的 `.jar`)、是否用 JRE System Library |
| **from Gradle** | `build.gradle` 路径、是否索引测试代码、中间依赖目录 |
| **from Maven** | `pom.xml` 路径、是否索引测试、中间依赖目录 |

⚠️ 📖 **Java 需要 Java 8 运行时**,且**位数必须和 Sourcetrail 一致**(64 位 Sourcetrail 要 64 位 JRE)。Windows 上的库文件是 `<jre>/bin/client/jvm.dll`,在 **偏好设置 → Java Path** 里指定。

### 4.6 Python

设置:Python 环境路径(可指向虚拟环境目录或 `python.exe`)、要索引的目录、排除、扩展名(`.py`)。
📖 还有 **Shallow Python Indexing**:只按**名字**解析引用(不精确但**快得多**),适合先粗索引一遍、边看边做第二遍细索引。

### 4.7 Custom Command Source Group

用来接入**用 [SourcetrailDB](https://github.com/CoatiSoftware/SourcetrailDB) 自己写的索引器**。给每个文件执行一条命令,支持这些占位符:📖

| 占位符 | 含义 |
|---|---|
| `%{SOURCE_FILE_PATH}` | 每个源文件的路径(**必需**) |
| `%{DATABASE_FILE_PATH}` | 工程数据库路径 |
| `%{DATABASE_VERSION}` | Sourcetrail 的数据库版本 |
| `%{PROJECT_FILE_PATH}` | 工程文件路径 |

### 4.8 通配符与环境变量(所有源组通用)📖

| 写法 | 含义 |
|---|---|
| `${ENV_VAR}` 或 `%ENV_VAR%` | **环境变量**(路径框里到处能用) |
| `*` | 匹配**除 `\` `/` 之外**的任意字符。例:`src/*/test.h` 匹配 `src/app/test.h`,**不**匹配 `src/app/widget/test.h` |
| `**` | 匹配**任意字符(含分隔符)**。例:`src**test.h` 上面两个都能中 |

💡 路径框的便利操作:点 `+`/`-` 增删、点 `...` 选文件、**直接从资源管理器拖进去**、点右下角**钢笔图标**当纯文本整批编辑。

---

## 5. 界面详解

### 5.1 主窗口的构成

| 部件 | 说明 |
|---|---|
| **子窗口(Subwindows)** | 三个视图各自是一个子窗口,**可在主窗口内自由拖动、也可以拖出去脱离主窗口**(多屏很有用)。每个子窗口有标题栏 + **关闭(x)** + **脱离(□)** 两个按钮。关掉的从 **View 菜单**重新打开 |
| **Tab Bar** | 顶部,用来**并行打开多个符号**。`+` 开新标签,`x` 关标签,**拖拽可调整顺序** |
| **状态栏(Status Bar)** | 底部,显示:最新状态消息、**编辑器插件连接状态**、**错误计数**、**索引进度条**。点消息数看 Status 标签、点错误数看错误位置、点进度条调出索引对话框 |
| **屏内搜索栏(On-Screen Search Bar)** | 用 **Ctrl+D** 调出,搜索**当前可见的** Graph 和 Code 内容(不是索引!)。右侧复选框决定搜哪个视图 |

### 5.2 搜索视图(Search View)

| 部件 | 作用 |
|---|---|
| **后退 / 前进** | 撤销 / 重做上一次导航(**Z** / **Shift+Z**) |
| **历史按钮** | 中间那个,列出**最近激活过的符号**栈,点一条即可跳回 |
| **刷新按钮** | 等价 **F5**(Refresh) |
| **Overview 按钮** | 回到工程概览(等价 **Ctrl+Home**) |
| **书签按钮** | 左=新建书签(**Ctrl+S**),右=书签管理器(**Ctrl+B**) |

**搜索栏的三种用法** ⭐:

| 输入 | 行为 |
|---|---|
| `RenderWeird` | **符号搜索**(模糊匹配,可以跳过字母;自动补全弹出会标出命中的字符和节点类型) |
| `?TODO` | **全文搜索**,不区分大小写(在**所有索引文件**里找字符串) |
| `??TODO` | **全文搜索**,区分大小写 |
| `overview` | 关键词:显示全部符号概览 |
| `error` | 关键词:显示所有错误 |

💡 自动补全框里:**上下箭头**选结果,**Tab** 把名字填进输入框,**Enter** 直接激活。

### 5.3 图视图(Graph View)—— 这是 Sourcetrail 的灵魂

**节点类型** 📖:

| 节点 | 说明 |
|---|---|
| **File** | 文件。⚠️ **斜纹/阴影的文件 = 未被索引**(不在任何源组里)或索引时**出过错** |
| **Macro / Namespace & Package & Module / Type / Typedef / Variable & Field / Function & Method / Enum & Enum Constant** | 各种符号 |
| **Class & Struct** | 成员**嵌套显示**,按 public/protected/private 分组;默认只显示**有边**的成员;箭头上的**数字**表示隐藏了多少个 |
| **Bundle** | **折叠节点** —— 把多个节点合成一个,名字说明是哪类,数字是数量 |
| **Group** | **分组节点** —— 把"同文件/同命名空间"的节点聚在一起 |

**边的类型** 📖:`File Include`、`Type Use`、`Variable Use`、`Function Call`、`Inheritance`、`Method Override`、`Template Specialization & Template Argument Use`、`Template Member Specialization`、**`Bundled Edges`(把两个节点子节点间的多条边打包,线越粗=条数越多,悬停能看到具体数量)**。

**默认配色**(📖 默认配色方案):

| 颜色 | 节点 | 边 |
|---|---|---|
| 灰 | 类型 / 类 | type use |
| 黄 | 函数 / 方法 | calls |
| 蓝 | 变量 / 字段 | variable access |

**操作**:
- 点节点 = 激活它(三个视图一起更新);拖节点 = 改位置;点类节点的箭头 = 展开/折叠;点 bundle = 展开
- 点边 = 在 Code 视图里高亮它的源码位置
- **平移**:拖背景 / 触控板滚动 / **W A S D**
- **缩放**:**Ctrl+滚轮**、**Ctrl+Shift+↑↓**、**0** = 重置缩放
- 右下角 **`?`** 打开 **Graph Legend**(也可以搜 `legend`)

**右键菜单** 📖(非常有用):**Open in New Tab**、Back/Forward、**Show Definition**、**Show Definition in IDE**(交给编辑器打开)、Expand/Collapse Node、**Hide Node / Hide Edge**、**Bookmark Node**、**Save As Image**(PNG/JPEG/BMP/**SVG**)、**Save To Clipboard**、**Copy Name**、**Copy Full Path**、**Open Containing Folder**。

#### Custom Trail(自定义路径)⭐

左上角工具栏可一键生成:

| 按钮 | 生成什么 |
|---|---|
| **Predefined Custom Trail** | **调用图 / 继承链 / include 树**(取决于当前符号类型) |
| **Custom Trail Dialog**(**Ctrl+U**) | 按自定义条件生成图 |

对话框里三种模式 📖:

| 模式 | 含义 |
|---|---|
| **To Target Symbol** | 只显示**从起点到终点**之间的路径 |
| **All Referenced** | 只显示**起点引用到的**节点 |
| **All Referencing** | 只显示**依赖起点的**节点 |

还可以设 **Maximum Depth(最大深度)**、**Layout Direction(横/竖)**、**Node Filters / Edge Filters**(只保留某些节点/边类型;特殊的 `member` 边代表父子关系)。

#### Node Grouping(节点分组)

左上角的两个按钮:**按 namespace/package 分组** 或 **按定义文件分组**。点组名可以激活对应的文件/命名空间节点。

### 5.4 代码视图(Code View)

两种模式 🔖:

| 模式 | 说明 |
|---|---|
| **Snippet List Mode**(默认) | 一次性列出当前符号的**所有引用**,最上面通常是**定义**。每个文件是一个"文件块" |
| **Single File Mode** | 一次只显示**一个完整源文件** |

文件块标题栏可点击切换三种状态 📖:**Minimized(只留标题)** → **Snippets(只显示相关片段)** → **Maximized(整个文件,进入 Single File Mode)**。

**交互**:
- **Ctrl+G / Ctrl+Shift+G** = 下一个 / 上一个**引用**
- **Ctrl+T / Ctrl+Shift+T** = 下一个 / 上一个**局部引用**(当前局部变量)
- 悬停时会**出现方框的符号可以点** → 激活它;点**局部的**符号 → 在可见代码里高亮它所有用处
- 片段顶部那行说明**所处作用域**,点它可以展开整个作用域
- ⚠️ **不能编辑代码**,只有语法高亮
- 文件图标带 **`x`** 说明它索引出过错,标题栏会有 **Show Errors** 按钮 → 只看这个文件的错误

### 5.5 状态视图(Status View)

| 标签 | 内容 |
|---|---|
| **Status** | 运行时的状态消息表(排查"为什么没按预期工作"很有用)。可以按类型过滤、`Clear Table` 清空;单元格**双击可选中复制** |
| **Errors** | 索引错误列表。列:**Type(ERROR / FATAL)**、错误消息、文件、行号、是否已索引、Translation Unit(哪个源文件产生的);点一行 → Code 视图定位;表头可排序;还有 `Edit Project` 按钮 |

> 📖 **FATAL 错误要特别注意**:它会让索引器**在这一处停下**,导致缺失大量信息。

### 5.6 工具提示(Tooltip)

悬停在符号上会显示:符号类型、可见性(public/private)、**被引用次数**、符号名、可点击的类型名、**完整函数签名**(返回值和参数类型都可点)。

### 5.7 书签(Bookmarks)

📖 书签信息存在工程文件旁边的 **`.srctrlbm`** 文件里。

| 动作 | 入口 |
|---|---|
| 给当前符号加书签 | **Ctrl+S** → 弹 **Bookmark Creator**(可填 **Name / Comment / Category**) |
| 管理书签 | **Ctrl+B** → **Bookmark Manager**;可激活、编辑、删除;按 **Category 分组**(没分类的进 `default` 组);可按"只看节点/只看边"过滤、可改排序 |
| 最近书签 | 菜单 **Bookmarks → Recent Bookmarks** |

---

## 6. 菜单逐条 📖

### Project
| 菜单项 | 作用 |
|---|---|
| New Project / Open Project / Recent Projects | 新建 / 打开 / 最近列表 |
| Edit Project | 打开工程设置(改源组、启用停用) |
| Exit | 退出 |

### Edit
| 菜单项 | 作用 |
|---|---|
| **Refresh**(F5) | 只重索引改动过的文件及其依赖 |
| **Full Refresh**(Shift+F5) | 全量重建(**也是"强制重索引出错文件"的办法**) |
| Find Symbol(Ctrl+F) | 光标放进搜索框 |
| Find Text(Ctrl+Shift+F) | 直接开始**全文搜索** |
| Find On-Screen(Ctrl+D) | 屏内搜索栏 |
| Next / Previous Reference(Ctrl+G / Ctrl+Shift+G) | 在引用之间跳 |
| Next / Previous Local Reference(Ctrl+T / Ctrl+Shift+T) | 在局部引用之间跳 |
| To overview(Ctrl+Home) | 回到概览 |
| Preferences(Ctrl+,) | 打开偏好设置 |

### View
New Tab / Close Tab / Select Next·Previous Tab / **Show Start Window** / **Show Title Bars** / **Reset window layout**(布局乱了点这个)/ Search·Graph·Code·Status Window(显隐四个子窗口)/ **Larger·Smaller·Reset Font**(Ctrl+ + / - / 0)

### History
**Back / Forward**(Z / Shift+Z)、**Recently Active Symbols**(按时间列出激活过的符号)

### Bookmarks
**Bookmark Active Symbol**(Ctrl+S)、**Bookmark Manager**(Ctrl+B)、Recent Bookmarks

### Help
About / **Keyboard Shortcuts**(弹出快捷键表)/ **Documentation**(官方文档 URL —— ⚠️ 网站已下线,打不开)/ Changelog / Bug Tracker / License / **Show Data Folder** / **Show Log Folder**

---

## 7. 完整快捷键表 📖

### 通用

| 功能 | Windows |
|---|---|
| 在 Graph / Code 视图间切换焦点 | `Tab` |
| 偏好设置 | `Ctrl + ,` |
| 新建 / 打开工程 | `Ctrl + N` / `Ctrl + O` |
| 刷新 / 全量刷新 | **`F5`** / **`Shift + F5`** |
| 后退 / 前进 | `Z` / `Shift + Z`(或 `Alt + ←/→`、`Backspace`) |
| 找符号 / 找文本 / 屏内找 | `Ctrl + F` / `Ctrl + Shift + F` / `Ctrl + D` |
| 回到概览 | `Ctrl + Home` |
| 新标签 / 关标签 | `Ctrl + T` / `Ctrl + W` |
| 下一个 / 上一个标签 | `Ctrl + Tab` / `Ctrl + Shift + Tab` |
| 字体 大 / 小 / 重置 | `Ctrl + +` / `Ctrl + -` / `Ctrl + 0` |
| 加书签 / 书签管理器 | `Ctrl + S` / `Ctrl + B` |

### 图视图

| 功能 | 快捷键 |
|---|---|
| 在节点间移动焦点 | `WASD` / `HJKL` / 方向键 |
| 在边之间移动焦点 | `Shift + WASD` / `Shift + HJKL` / `Shift + 方向键` |
| 激活节点/边 | `Enter` 或 `E` |
| 在新标签里激活 | `Ctrl + Shift + Enter` / `Ctrl + Shift + E` |
| 展开/折叠节点 | `Shift + Enter` / `Shift + E` |
| 平移 | `Ctrl + 方向键` |
| 放大 / 缩小 | `Ctrl + Shift + ↑/↓` 或 `Ctrl + 滚轮` |
| 重置缩放 | `0` |
| 打开 Custom Trail 对话框 | `Ctrl + U` |

### 代码视图

| 功能 | 快捷键 |
|---|---|
| 下一个 / 上一个引用 | `Ctrl + G` / `Ctrl + Shift + G` |
| 下一个 / 上一个局部引用 | `Ctrl + T` / `Ctrl + Shift + T` |
| 在代码间移动焦点 | `WASD` / `HJKL` / 方向键 |
| 跳到最近的引用 | `Shift + WASD` / `Shift + HJKL` / `Shift + 方向键` |
| 激活位置 | `Enter` 或 `E` |
| 在新标签里激活 | `Ctrl + Shift + Enter` / `Ctrl + Shift + E` |
| 滚动 | `Ctrl + 方向键` |

---

## 8. 偏好设置(Preferences)逐项 📖

`Edit → Preferences`(**Ctrl+,**)。这些设置是**所有工程共用**的。

| 设置 | 说明 |
|---|---|
| **Font Face / Font Size** | 界面字体与字号 |
| **Tab Width** | 代码视图里 Tab 显示成几个空格 |
| **Text Encoding** | 显示源码用的编码。⚙️ **本机是 `UTF-8`** —— 所以**我们的中文注释能正常显示** ✅ |
| **Color scheme** | 配色方案(文件在 `data/color_schemes/`;自带 `dark`、`bright`、`solarized_dark`、`solarized_light`、`bad_rainbow`) |
| **Animations** | 界面动画开关 |
| **Built-In Types** | `int`/`bool` 这类内置类型要不要出现在图里(⚙️ 本机=关) |
| **Directory in File Title** | 文件标题里显示父目录(⚙️ 本机=关) |
| **Scroll Speed** | 滚动速度倍数(<1 变慢,>1 变快) |
| **Graph Zoom** | 鼠标滚轮默认是**平移**还是**缩放** |
| **Logging** | 写日志到控制台和文件(**默认关闭以提速**;出问题查原因时打开) |
| **Indexer Logging** | 索引期间的详细日志(给开发组排查用) |
| **Automatic Update Check** | 每天自动检查新版本(⚠️ 项目已停更,没意义) |
| **Sourcetrail Port / Plugin Port** | 与编辑器插件通信用。⚙️ 本机 = **6667**(监听)/ **6666**(发送) |
| **Indexer threads** | 索引并行线程数,`default` = 按 CPU 自动(⚙️ 本机=0 即 default) |
| **Multi process C/C++ indexing** | C/C++ 索引丢到**独立进程**,避免索引时崩掉整个程序(⚙️ 本机=**开**) |
| **Java Path / JRE System Library / Maven Path** | 索引 Java 才需要 |
| **Post Processing** | 索引后跑一遍"后处理"解决未解析引用(标成 ambiguous) |
| **Global Include Paths** | **所有工程**共用的系统头路径。⚠️ ⚙️ **本机这里填的是旧的**:`MSVC 14.44.35207` + `Windows SDK 10.0.19041.0`(而实际编译用的是 `14.38` + SDK `10.0.26100`)—— **对我们没影响**,因为我们的编译数据库里带了显式路径 ✅ |
| **Global Framework Search Paths** | (仅 macOS) |

---

## 9. 编辑器插件(含 Emacs!)📖

Sourcetrail 可以和编辑器**双向联动**(通过本地 TCP:**Sourcetrail 监听 6667,发往 6666**)。

### 9.1 从 Sourcetrail → 编辑器

在 Code 视图里**按住 `Ctrl` 点某一行**,或右键菜单选 **Set IDE Cursor** → 编辑器跳到那个位置。

### 9.2 从编辑器 → Sourcetrail

**Emacs**(⭐ 你就在用 Emacs):

| 步骤 | 做法 |
|---|---|
| 安装(手动) | 下载 [`sourcetrail.el`](https://github.com/CoatiSoftware/emacs-sourcetrail) → Emacs 里 `M-x package-install-file` → 填文件路径 |
| 安装(Melpa) | 加 Melpa 源 → `M-x list-packages` → 找 `sourcetrail` → `i` 标记 → `x` 安装 |
| **发送位置** | 光标放到符号上 → **`M-x sourcetrail-send-location`** |
| **接收位置** | 先启动 **`M-x sourcetrail-mode`**(这样 Sourcetrail 点行时才会跳过来) |

> ⚠️ 官方说插件在**下载包的 `/ide_plugins` 目录**里。本机是 `setup.exe` 装的,**没有那个目录**,所以要用上面 GitHub 的地址取。

**其它编辑器**(同一个仓库组织下):Atom、CLion/IntelliJ IDEA、Eclipse、Qt Creator、Sublime Text 2/3、Vim(`:SourcetrailActivateToken`)、VS Code(`Sourcetrail: Send Location`)、Visual Studio(2012–2017)。

### 9.3 VS 插件还能**导出编译数据库** ⭐

Visual Studio 插件里有个向导(在 VS 菜单栏的 Sourcetrail 区):勾选要包含的 C/C++ 项目、选 **Configuration / Platform**、选输出目录和文件名 → **Create** 就能生成 `compile_commands.json`。
📖 官方特意说:**这个格式与 Sourcetrail 无关**,生成的数据库也能给**其它 clang 系工具**用。

### 9.4 IDE 通信协议(想自己写插件/脚本才需要)📖

消息格式:`messageType>>参数>>…>>参数<EOM>`(`>>` 是分隔符,`<EOM>` 是结束符)

| 方向 | 消息 | 参数 |
|---|---|---|
| **Sourcetrail → 插件** | `moveCursor` | `fileLocation`, `row`, `column` |
| | `createCDB` | (无) |
| | `ping` | (无) |
| **插件 → Sourcetrail** | `setActiveToken` | `fileLocation`, `row`, `column` |
| | `createCDBProject` | `cdbPath`, `headerPaths`(多个用 `>>` 分隔) |
| | `ping` | (无) |

---

## 10. 文件格式与版本控制建议

| 文件 | 内容 | 建议 |
|---|---|---|
| `HandMade.srctrlprj` | XML,`<version>8</version>`,源组配置 | ✅ **提交** |
| `compile_commands.json` | 编译数据库(clang 格式) | ✅ **提交**(它同时还能给 clangd / VS Code / Emacs 用) |
| `HandMade.srctrldb` | SQLite 索引库 | ❌ **加进 `.gitignore`**(可重建,而且会很大) |
| `HandMade.srctrlbm` | 书签 | 🟡 随意 |

⚠️ **`.gitignore` 目前还没加 `.srctrldb`** —— 需要的话跟我说一声(改仓库文件按 `NOTES.md` 要你点头)。

---

## 11. 常见问题与排查

| 现象 | 原因 / 解决 |
|---|---|
| **打开工程一片空白** | 还没索引 → 点 **Start**(或 F5) |
| **改了代码但图没变** | 忘了 **Refresh(F5)**;或文件内容没变(那就 **Shift+F5** 全量) |
| **某些符号点不动 / 被引用数为 0** | 索引出错 → 看 **Status View → Errors**;FATAL 错误尤其要注意 |
| **`#include <windows.h>` 报找不到** | 编译数据库里缺头文件路径。检查 `compile_commands.json` 的 `-isystem`,或补 **Global Include Paths** |
| **索引特别慢** | ① 别索引系统/第三方头(用 `indexed_header_paths` 限定到自己的代码)② 调 **Indexer threads** ③ 打开 **Multi process C/C++ indexing** |
| **图里全是 `int`/`bool` 之类的噪声** | 关掉偏好设置里的 **Built-In Types** |
| **中文注释显示成乱码** | 偏好设置 → **Text Encoding** 改成 `UTF-8`(⚙️ 本机已经是) |
| **布局乱了** | 菜单 **View → Reset window layout** |
| **想排查为什么某项功能不工作** | 偏好设置里打开 **Logging** + **Indexer Logging**,然后 **Help → Show Log Folder** |
| **Java 工程索引不了** | 需要 **Java 8**,且**位数要和 Sourcetrail 一致**;在偏好设置指定 `jvm.dll` |
| **更新检查失败** | 项目已停更,网站也下线了 —— 忽略即可 |

---

## 12. 本仓库(HandMade)专用速查

```cmd
:: 打开(双击也行)
D:\HandMade\HandMade.srctrlprj

:: 命令行索引
"C:\Program Files\Sourcetrail\Sourcetrail.exe" --index "D:\HandMade\HandMade.srctrlprj"

:: 全量重建
"C:\Program Files\Sourcetrail\Sourcetrail.exe" --index --full "D:\HandMade\HandMade.srctrlprj"
```

**值得在 Graph 里点一圈的符号**(我们当前的代码):

| 符号 | 看什么 |
|---|---|
| `WinMain` | 整个程序的起点,往下会连到 `RegisterClass` / `CreateWindowEx` / 消息循环 |
| `WindowMainWindowCallback` | 窗口过程,连到 `Win32ResizeDIBSection` 和 `Win32DisplayBufferInWindow` |
| `RenderWeirdGradient` | 像素循环 |
| `Win32ResizeDIBSection` | `VirtualAlloc` 那块内存的来处 |
| `win32_offscreen_buffer` | 结构体类型 —— 看**谁用了它**(`Type Use` 边) |

**改了代码之后的标准流程**:

```
① 改 .cpp
② D:\HandMade\HandMade\build.bat        ← 先确认还能编过(这一步和 Sourcetrail 无关,但别偷懒)
③ Sourcetrail 里按 F5(Refresh)
④ 如果改了 include 结构/加删了文件 → 用 Shift+F5(Full Refresh)
```

> 💡 因为我们的编译数据库**是静态手写的**(不像 CMake 会自己更新),所以:
> **只在 `build.bat` 的编译参数变化时**(比如改了头文件搜索路径、加了库),才需要**重新生成 `compile_commands.json`**。
> 加删源文件**不需要**改数据库 —— 但**单一源文件的命令**意味着:以后项目拆成多个 `.cpp` 时,**必须给每个源文件各写一条记录**(这份文档写的时候我们还只有一个文件)。

---

## 13. 附录:官方资料位置

| 资源 | 地址 / 位置 |
|---|---|
| **官方完整文档**(本文的主要来源) | 仓库根目录的 [`DOCUMENTATION.md`](https://github.com/CoatiSoftware/Sourcetrail/blob/master/DOCUMENTATION.md)(1,684 行) |
| 代码仓库(已归档) | <https://github.com/CoatiSoftware/Sourcetrail> |
| 发布页(下载) | <https://github.com/CoatiSoftware/Sourcetrail/releases> |
| Bug Tracker(已归档) | <https://github.com/CoatiSoftware/SourcetrailBugTracker> |
| 自研索引 API | <https://github.com/CoatiSoftware/SourcetrailDB> |
| Emacs 插件 | <https://github.com/CoatiSoftware/emacs-sourcetrail> |
| 本机官方文档副本 | `%TEMP%\Sourcetrail_DOCUMENTATION.md`(169 KB,从 GitHub API 取回) |

> ⚠️ 注意:`Help → Documentation` 菜单里那个网址指向的官网**已经下线**,点了打不开 —— 要看文档就用上表的 GitHub 链接。

---

## 附:本文的核查清单

| 结论 | 依据 |
|---|---|
| 安装路径、数据目录、文件清单 | ✅ 实际列目录 |
| 版本 2021.4、仅 C/C++/Java/Python、Clang 11 驱动 | 📖 官方 DOCUMENTATION.md |
| 全部界面/菜单/快捷键/设置项/源组类型/插件/协议 | 📖 官方 DOCUMENTATION.md |
| 命令行参数(`--index` / `--full` / `--incomplete` / `--shallow` …) | ✅ 从 `Sourcetrail.exe` 字符串提取(官方文档**未**记载) |
| `.srctrlprj` 格式(`version 8`)、字段名合法 | ✅ 与本机可用的 `Piccolo.srctrlprj` / `tictactoe_cpp.srctrlprj` 逐字段对照,并在 exe 中核对了字段名 |
| **我们的项目索引成功** | ✅ `HandMade.srctrldb` 是合法 SQLite(218 KB),内含 `RenderWeirdGradient` 等符号 |
| `compile_commands.json` 正确 | ✅ `clangd --check` 解析零语法/语义错误 |
| 本机偏好设置各项当前值 | ⚙️ `ApplicationSettings.xml` |

> 文档整理时间:2026-09-21
