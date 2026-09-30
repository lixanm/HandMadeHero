# 0004 — 不链 `xinput.lib`:手柄走运行时 `LoadLibraryA` + `GetProcAddress`

- **状态**:已采纳
- **日期**:2026-09(Day 006 落地时;确切日期未记 🟡)
- **相关**:`HandMade\code\win32_handmade.cpp`(手柄那一段、宏 + stub + 函数指针)、
  `explain\!HandmadeHeroExplain\Day 006 - Gamepad and Keyboard Input.md`、`README.md` 的"链接的库"一节

## 背景

手柄输入要用 XInput。最直觉的做法是**在工程里链上 `xinput.lib`** —— 但那意味着 Windows **加载器在进程启动时**
就要解析导入表,于是没有 `xinput1_3.dll` 的机器上,**游戏连窗口都打不开**(不是"手柄不能用",是整个程序起不来)。

这是个"代价与收益完全不成比例"的失败模式:绝大多数目标机器都装了 XInput,但那少数没装的机器会得到
最糟糕的结果 —— 一个打不开的程序,而不是一个没有手柄支持的程序。

## 决定

**不链 `xinput.lib`,改成运行期动态链接:**

1. `Win32LoadXinput()` 里 `LoadLibraryA("xinput1_4.dll")`,失败再试 `xinput1_3.dll`(**分层加载**:
   Win7 只有 1.3、Win8+ 只有 1.4;这台机器两个都有,所以现在不会失败,但代码要跟上);
2. `GetProcAddress` **按名字**取导出函数 —— ⚠️ **不能按序号**(1.4 / 1.3 / 9_1_0 的导出序号不同,6/6/4);
3. 手写函数指针类型 + **stub 兜底**:取不到地址时函数指针指向桩,桩返回 `ERROR_DEVICE_NOT_CONNECTED`(1167);
4. 用 `#define XInputGetState XInputGetState_` 把调用点重定向到函数指针(不 `#include <xinput.h>` 的实体声明);
5. 故意**不 `FreeLibrary`**(进程退出时一起还)。

## 后果

**好的**

- **缺 DLL 的机器也能启动**,只是手柄不可用 —— 失败被降级到正确的层级;
- 同一套代码可以跨 XInput 世代(1.4 / 1.3)自适应;
- 学到了"导出函数的地址可以在运行期成为一个数据"(这条经验后来直接用在 DirectSound 上)。

**代价 / 需要盯着的**

- 多一层间接调用;每个 API 都要写 stub 与函数指针,样板代码多;
- 必须**按名字**取,且要防"某个 DLL 少导出某个函数";
- 这条经验**不能无脑推广**:DirectSound 的接口方法**没有**导出名字可用,只能靠 COM 的 vtable
  (见 `explain\!HandmadeHeroExplain\Day 008 ...md` §2.4)。**"导出函数 → 按名字取;接口方法 → 走 vtable"**;
- 以后加 D3D 之类时,同样的取舍要重做一次(见 §"替代方案")。

## 考虑过的替代方案

| 方案 | 为什么不选 |
|---|---|
| 链 `xinput.lib`(静态导入) | 缺 DLL 的机器**整个游戏打不开** —— 正是要避免的那个失败模式 |
| `/DELAYLOAD:xinput1_3.dll`(延迟加载) | MSVC 支持,启动也不崩;但**第一次调用**仍需处理失败(要写 delay-load 钩子),样板不比手写少,而且隐藏了"什么时候会失败"。🟡 没实测过本项目下的行为 |
| 只支持 `xinput9_1_0.dll`(老版本) | 功能少(没有振动等);Casey 的做法是**按世代从新到旧试** |
| 直接 `GetProcAddress` 不写 stub(失败即崩) | 违背"缺东西也要能启动"的初衷 |
