# Handmade Hero 的终点与导航 —— 这个系列最后到底做出了什么

> **本文件**：`C:\THOTH\DaiMaXuNiJi\NEWCODE\HandMade\explain\HANDMADE_HERO_终点与导航.md`
> **用途**：回答两个问题 —— （1）跟着写到底会得到什么样的成品？（2）想看"最成品"的样子，该看哪几集？
> **查证日期**：2026-09-19
> **素材来源**：官方 episode guide（`guide.handmadehero.org`）、Molly Rocket 官网、handmade.network 论坛
> **⚠️ 重要声明**：写作时这台机器上 `youtube.com` 域名被 DNS 拦（解析到非公网 IP），**我没有看过任何一集视频**。下面关于"长什么样"的描述全部来自 episode guide 的**逐日标题和时间轴文字**、以及官网/论坛的公开说明；凡是推断而非直引的地方我都标了"（推断）"。画面的具体观感请以视频为准。

---

## 一、结论：这个项目没有"最终成品"

| 事实 | 证据 |
|---|---|
| **项目已退休**，官方原文：*"Handmade Hero has been retired."*（还给早期预购者提供退款） | [mollyrocket.com](https://mollyrocket.com/) |
| `handmadehero.org` 现在 **302 跳转**到 mollyrocket.com（官网只留一个入口） | 直接访问 `handmadehero.org` 的响应 |
| 主线**最后一集是 Day 667**「Simplified Tile Occupancy Checking」 | [guide.handmadehero.org/code/day667](https://guide.handmadehero.org/code/day667/) |
| 那一页底部原话：**"You have arrived at the (current) end of Handmade Hero"** | 同上 |
| 开播：**2014 年 11 月 17 日**；最初估计"至少 600 集"；工作节奏每天一小时 | [hero.handmade.network](https://hero.handmade.network/) |
| 最后一集大约停在 **2023 年**（2024-09 有人写总结时已停更一年半，并说"667 集，其实还多一点"） | [Handmade Hero, конец](https://hndmd.ru/archive/tg/2024/0036-0-handmade-hero-the-end) |
| 代码规模参考：**Day 638 时约 42,402 行** | [论坛帖](https://hero.handmade.network/forums/game-discussion/t/8644-is_handmade_hero_ever_going_to_finish) |
| 社区一直在问"到底会不会做完"，答复是"他一周只播一次、还常跳" | 同上 |
| 项目页仍写"ongoing project"，但论坛活跃度已经从 2023 一路降到零星 | [项目 Feed](https://hero.handmade.network/posts) |

**所以"最终成品"= Day 667 那一版 exe：一个能跑、能编辑、有动态全局光照的地牢 crawler 引擎，但它没有游戏内容**（没有剧情、没有关卡设计、没有成品美术量）。HN 上的批评很直白：十年下来"只是一个 rudimentary, tile-based dungeon-crawler engine"，并且作者当年确实收过预购（[HN 讨论](https://news.ycombinator.com/item?id=44611923)）。

### 集数是怎么构成的

| 系列 | 说明 |
|---|---|
| **Handmade Hero（主线）** | **Day 001 – Day 667**，就是我们现在跟的这个 |
| Q&A | 大多数天后面跟一集 `dayXXXqa`，专门回答问题 |
| Intro to C on Windows | 入门系列，Day 1–5（+ 各集 Q&A），讲 C 和 Win32 基础 |
| Handmade Chat | 闲聊/答疑专场，能查到的编号 001–016（中间有缺号） |
| Handmade Ray / Handmade Miscellany / HandmadeCon（2015、2016） | 侧线系列，独立于主线 |

---

## 二、Day 667 那一版由什么组成

按子系统列一遍（**左列结论都来自对应日期的标题/时间轴，可逐条回查**）：

| 子系统 | 到最后一集时的状态 | 关键日期 |
|---|---|---|
| 平台层 | Win32 单文件平台层 + 动态重载 DLL、热重载、自建 arena 内存、**不用 CRT** | 001–025、305、341–347、435、575 |
| 渲染 | 软件渲染器（自写 SIMD 光栅化）**和** OpenGL 后端并存；透视投影 + 精灵卡片 billboard + 深度缓冲 + 多重采样 + 自定义 depth peel resolve | 088–111、235–263、316–324、358–388、556 |
| 世界 | 程序化生成：房间、门、楼梯、**overworld 布局**并把实体自动摆进世界 | 444–452、485–487、502、627、659–662 |
| 角色 | 主角 + **Glove（拳套）**、移动队列、按体素/球体的碰撞解算、弹簧动画 | 273–276、294、631–637、655–658 |
| 敌人/AI | **Brains** 行为系统、Familiars（伙伴）、Monstar、Snakes、traversable 占用 | 059、287–288、291–293、331、628 |
| 光照 | **体素光照探针 + 八面体光照图 + 无限次反弹的间接光**，kd-tree / grid raycast 加速，**filmic 响应曲线**，**自发光地砖** | 389–438、547–618、646–654 |
| 分层/观感 | Z 层堆叠（房间上下层）、层间雾、按图形（graph）的渲染排序、sort rule | 295–315、375 |
| 工具 | 游戏内编辑器（对齐点、撤销重做、直接改资产并写回）、调试 UI/树、性能分析器、内存占用可视化、图表式调试绘制 | 176–263、488–506、539–544、638–644 |
| 资产 | 自写 **PNG 解码器**（zlib/Huffman/滤波）、WAV、字体提取器 + PNG 写出、HHA/HHT 资产格式 | 147–154、453–470、509–533 |
| 音频 | 自写混音器（SSE）、逐样本插值、变调、流式播放 | 007–009、138–146、526 |

**一句话概括观感（推断）**：2D 像素精灵站在一个真 3D 投影的场景里，房间按 Z 层堆叠、层间有雾，地牢是程序化生成的，光线会在地砖和墙之间来回反弹、自发光地砖把地板照亮；屏幕上是自制的调试/编辑器 UI（树状层级 + 时间轴图 + 内存可视化），因为这套东西本来就是拿来教学的。

**但它不是"一个游戏"**：没有关卡设计、没有数值/流程、没有菜单和存档——本质上是一个被人玩了十年的**引擎 + 工具集**。

---

## 三、从我们现在的 Day 3 到 Day 667：里程碑地图

> 我们仓库现在的状态：Day 1–3，`win32_handmade.cpp`（144 行）——一个窗口 + 一块还没用起来的后备缓冲 + `PatBlt` 黑白闪烁。
> 下表就是"从这里到终点"之间会发生的事。**每一段都可以拿去 guide 里回查对应集数。**

| Day 区间 | 这一段在干什么 | 你会在这一段学到 / 得到 |
|---|---|---|
| **001–025** | **平台层**：窗口 → 后备缓冲 → 输入 → DirectSound → 高频计时 → 平台 API 设计 → 动态加载游戏代码（热重载） | ← **我们在这里**。这一段结束时，游戏逻辑就和 Win32 解耦了 |
| 026–070 | 游戏架构：tile map → 稀疏存储 → 位图加载/混合 → 向量与运动方程 → Minkowski 碰撞 → 实体空间分区 → **sim region** → Familiar / 血量 / 攻击 / 抛射物 | "探索式架构"的骨架，实体系统 |
| 071–111 | **3D 化 + 渲染管线**：全面 3D 定位、多碰撞体、上下楼梯、ground buffer、push buffer 渲染、sRGB 与光照线性空间、法线图、透视投影、分辨率无关渲染 | 从"贴图"变成"真 3D 场景 + 光照" |
| 112–131 | 性能与并发：CPU 性能心智模型、性能计数器、**SIMD 光栅化**、多线程 + 工作队列、正交投影、异步地面块合成 | Casey 的 SIMD / 多线程招牌内容 |
| 132–175 | 资产与音频：资产流式加载、自写 WAV、**混音器（含 SSE）**、资产文件格式、粒子入门、通用分配器、字体（含 kerning） | 自造资产管线 |
| 176–234 | 调试基础设施 + 元游戏：debug 事件/层级/自省（introspection）、径向菜单、运行时重编译、**cutscene（过场）**、metagame 模式切换、渲染排序 | 这套调试 UI 是 HH 最独特的部分 |
| 235–263 | **OpenGL** 接入 + 分析器：OpenGL 上下文、异步纹理上传、profile tree | 第三个渲染层 |
| 264–330 | 工具与系统深化：分析器 UI、traversable 混合移动、**弹簧动画**、稀疏实体系统搬进 world chunk、房间间相机、**Brains**、Monstar/Snakes/Familiar、**Glove**、Z 层与雾、排序（sort key → sort rule → graph sort → 网格加速）、多渲染目标、ticket mutex、自写 printf | 游戏"手感"和渲染排序的大头 |
| 331–378 | 3D 渲染正式化：Brains 激活、boost pad、粒子缓存 + SIMD、动态 arena 与内存保护页、**多线程世界模拟**、RLE/LZ 压缩、**深度缓冲**、3D 旋转矩阵、轨道调试相机、**着色器/核心 profile**、雾、调试体积 | 现代 GL 管线 |
| 379–406 | **depth peeling + 光照起步**：两遍剥离、自定义多重采样 resolve、Phong 光照、multigrid 光照、AO、光线投射、体素索引光照样本 | 光照研究的起点 |
| 407–470 | **体积光照 + 世界生成 + 自写 PNG**：体积纹理、光照追踪、函数近似（嘉宾 Andrew Bromage）、世界生成器（房间体积/相邻/连接）、**自写 PNG 解码器**、从 PNG 网格切资产 | 难度陡增但极有代表性的一段 |
| 470–545 | **渲染器彻底分离 + 游戏内编辑器**：独立渲染层、立方体 UV、世界生成摆实体、**编辑器（撤销重做/对齐点/写回 HHA）**、LRU 纹理句柄、HHT 标签解析、Git 入门、WAV/字体导入、粒子/场景/道具、内存可视化、地面植被 | "能编辑世界"的那一版 |
| 546–618 | **光照的最终形态**：光照探针 → 纯体素光照 → 探针光线投射 → **八面体光照图** → **无限次反弹** → 相机对齐光照体素 → kd-tree / grid raycast 优化 → 半分辨率空间网格 | 全系列技术含量最高的一段 |
| 619–667 | 收尾：资产标签哈希、光照 bug、traversable、用 tile 开门、Brains and Aprons、**重做碰撞（体素碰撞 + 碰撞球）**、图表式调试、去掉光照里的 Z、**filmic 曲线**、**自发光地砖**、**move queue**、Glove 碰撞、**即时模式关卡生成 + overworld 布局 + 从布局生成实体**、实体存储简化 | 最后一集 Day 667 是"简化 tile 占用检查"——纯重构，画面上没什么可看的 |

---

## 四、想看"最成品"的样子：观片清单

主线视频在 YouTube 上**免费**，你给的那个播放列表（`PL0PAV3gVZ9gmiTxKufnvxw2-WMFHTMX6c`）一直排到 Day 667，**列表最后一条就是终点**。想跳过 660 集直接看画面，按下面的挑：

| 想看什么 | 看哪几集 | 为什么 |
|---|---|---|
| **光照最终的观感** | **Day 653** Filmic Response Curve、**Day 654** Self-illuminating Floor Tiles | 这两集之后画面基本定型：色调映射 + 会发光的地砖 |
| **角色移动 / 攻击手感** | **Day 655–658**（Revisiting Entity Movement → Move Queues → Glove Collisions） | 主角真正"动得像样"的最后一段 |
| **世界生成（最大场面）** | **Day 659–662**（即时模式关卡生成 → 随机 overworld 布局 → 连接 overworld → 从布局生成实体） | 最后一次能肉眼看到"世界长出来、实体被摆进去" |
| **最像"游戏"的一段** | **Day 488–506**（游戏内编辑器 + 相机缓动） | 有 UI、有编辑、有相机运动，观感最热闹 |
| **光照技术的核心** | **Day 576–586**（八面体编码/光照图 → 无限次反弹）、**Day 600–617**（grid raycast 优化） | 想搞懂那套 GI 就看这里 |
| **纯重构、可跳过** | Day 663–667 | entity 存储重构，画面无变化 |

**精确跳转**：官方 episode guide 给每天做了**逐时间戳索引**，可以直接跳到你关心的那一段，不用拖进度条 —— <https://guide.handmadehero.org/code/day667/>（把 `day667` 换成任意天即可，例如 [day654](https://guide.handmadehero.org/code/day654/)）。

另外，Casey 真正拿得出手的商业成品 —— Molly Rocket 的 **"Secret Game Project"** —— 只给发行/平台方私下 demo，**没有公开视频**；他现在的精力主要在 [computerenhance.com](https://computerenhance.com) 的性能课程上。

---

## 五、能不能自己把它跑起来？

| 问题 | 答案 |
|---|---|
| 源码能拿到吗 | **能**：官网上还写着每日源码归档仍可从 SendOwl 购买，并给了丢失下载链接的找回页面 |
| 授权是什么 | **个人、不可转让、非商业、不可再分发**（[Handmade Hero License](https://hero.handmade.network/blog/p/7-handmade_hero_license)，2016-04-23）。作品/音乐版权另有约束 |
| 那为什么 GitHub 上没有完整的社区复刻版 | 就是因为上面的授权。能找到的都是**跟着写到某一天的学习仓库**（如 [cj1128/handmade-hero](https://github.com/cj1128/handmade-hero)、[bruno-sartori/handmade-hero](https://github.com/bruno-sartori/handmade-hero)），不是完整版 |
| 作者推荐怎么用这个系列 | 别复制代码，**自己手打一遍**（见 [Handmade Hero, конец](https://hndmd.ru/archive/tg/2024/0036-0-handmade-hero-the-end) 的总结）——也就是我们这个仓库现在在做的事 |
| 还有人在讨论吗 | 有，论坛一直在用（2026 年还有新帖），提问前先搜：[代码讨论区](https://hero.handmade.network/forums/code-discussion) |

---

## 六、链接汇总

| 资源 | 地址 |
|---|---|
| 项目主页（Handmade Network） | <https://hero.handmade.network/> |
| 每日 episode guide（含逐时间戳索引） | <https://guide.handmadehero.org/> |
| 最后一集（终点） | <https://guide.handmadehero.org/code/day667/> |
| Molly Rocket 官网（退休公告 / 源码购买入口） | <https://mollyrocket.com/> |
| 授权说明 | <https://hero.handmade.network/blog/p/7-handmade_hero_license> |
| 论坛：会不会做完 | <https://hero.handmade.network/forums/game-discussion/t/8644-is_handmade_hero_ever_going_to_finish> |
| 论坛：代码讨论区（提问用） | <https://hero.handmade.network/forums/code-discussion> |
| Casey 现在的课程 | <https://computerenhance.com> |

---

## 七、这份文档对我们仓库的意义

1. **别指望"跟着写完就有个游戏"** —— 这条线的终点是一个引擎，Day 667 之后没有彩蛋。
2. **Day 1–25 是我们必须自己走过去的第一段**（平台层），走完之后游戏逻辑才和 Win32 分开，那时我们这个仓库的结构才会变成"平台层 + 游戏层"两份文件。
3. **想提前看画面**：按第四节清单看 Day 653–662 那几集即可，不必等 660 集。
4. 我们现在的 README 进度表只到 Day 3，等真正开工 Day 4/5 时，把本文件当作"远期地图"对照着看即可。
