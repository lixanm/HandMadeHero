# 0003 — VS2022 是唯一构建真源;`build.bat` 降级为包装

- **状态**:已采纳
- **日期**:2026-09-30
- **相关**:`HandMade\HandMade.vcxproj`、`HandMade\build.bat`、`debug.bat`、`README.md`、`explain\2026-9\2026-9-30.md`

## 背景

在此之前有**两套构建配置**,而它们并不一致:

| 项目 | 旧 `build.bat`(命令行 cl) | 旧 VS 工程(向导默认) |
|---|---|---|
| 警告等级 | `/W4` | `Level3` |
| **警告当错** | `/WX` | **没开** |
| 屏蔽警告 | 7 个 | 无 |
| 预处理宏 | `HANDMADE_SLOW/INTERNAL/WIN32` | 只有 `_DEBUG` |
| 运行库 | `/MTd`(静态 CRT) | `/MDd`(动态) |
| 链接库 | `user32 gdi32 winmm` | 缺 `winmm` |
| 输出 | `HandMade\build\win32_handmade.exe` | `x64\Debug\HandMade.exe` |

"在 VS 里能过、命令行过不了"这类问题**迟早会出现**,而且两边都"看起来对",极难查。
触发这次决定的直接原因:用户宣布**以后不再用 Emacs,只用 VS2022**。

## 决定

1. **`HandMade\HandMade.vcxproj` 是编译/链接设置的唯一真源**;
2. 把 Casey 的那套参数**搬进工程**:`/W4` + `/WX` + 7 个 `-wd` + `/FC` + `/MTd`(`Release` 为 `/MT`)
   + `/GR-` + `/EHa-` + `/Oi` + `user32/gdi32/winmm` + `HANDMADE_*` 宏 + `/INCREMENTAL:NO`;
3. **`build.bat` 降级**为"一行 msbuild 的包装",**不再持有任何编译选项**(两处的名字仍保留,
   因为仓库里约 40 处文档引用它;但它已不可能与工程跑偏);
4. `debug.bat` 的产物路径改为 `x64\Debug\HandMade.exe`;`exe` 名统一为 **`HandMade.exe`**;
5. 输出位置:**仓库根**的 `x64\<Config>\`(MSBuild 解决方案级输出),不在项目文件夹里。

## 后果

**好的**

- 两个入口产出的东西**完全一致**;"哪套配置说了算"不再是一个问题;
- 运行库改回静态 `/MT` ⇒ exe 不依赖 VC 运行库 DLL(`dumpbin /dependents` 实测:只有 `USER32/GDI32/KERNEL32`);
- 警告当错 ⇒ 警告重新变成"必须处理的事",而不是背景噪音。

**代价 / 需要盯着的**

- VS 工程另外开着 **`/permissive-`**(比 Casey 的参数更严)。目前代码能过;
  哪天它报了 cl 不报的错,那是**严格度**差异而不是代码错 —— 可以把「符合模式」关掉对齐;
- **`exe` 路径变了**:任何写死 `HandMade\build\win32_handmade.exe` 的地方都要改
  (`debug.bat` 已改;旧的 `build\` 目录已废弃,属 `.gitignore`);
- `HANDMADE_*` 宏目前代码里**还没有用到**(是为后面几集预备的),别以为它们现在在起作用。

## 考虑过的替代方案

| 方案 | 为什么不选 |
|---|---|
| 反过来:保留 `build.bat` 为真源,VS 工程只当编辑器 | 用户已决定只用 VS2022;让 VS 跑一套与真源不同的参数,等于把漂移留在常用路径上 |
| 删掉 `build.bat` | 可行(用户也表示"可保留或删"),但仓库里约 40 处文档引用它 —— 保留成空壳的代价更低 |
| 只对齐最要命的几项(警告当错/宏/运行库) | 剩下的差异(链接库、输出名、严格度)照样会在某天变成"为什么命令行能过 VS 不能" |
