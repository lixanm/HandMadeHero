# Emacs 键位表(VS2022 风格 + Casey 层)

> **对应配置**:`C:\Users\thoth\AppData\Roaming\.emacs.d\init.el`(**967 行 / 12 节**,最后修改 2026-09-18 11:33)
> **Emacs**:30.2,免安装版 `C:\THOTH\RuanJian\emacs-30.2`
> **本表生成**:2026-09-18
>
> ⚠️ **这张表不是照抄配置文件里的文字,而是用真实 Emacs 30.2 加载你的 init.el 之后逐键实测导出的**
> (命令见第 9 节,你可以随时自己重跑)。凡是配置注释和本表冲突,**以本表为准,或者用 `C-h k` 现查**。
>
> **改了配置要生效**:重启 Emacs,或在 Emacs 里按 `M-x eval-buffer`。
> **不想要 VS 风格键位**:`M-x my-vs-keys-mode`(关掉这一层);**不想要 CUA 复制粘贴**:`M-x cua-mode`。

---

## 0. 先看这三条(其他都是锦上添花)

1. **迷路 / 按错 → `C-g`**(随时脱身)
2. **记不住键 → `M-x`** + 打几个字母(Tab 补全),所有功能都能找到
3. **想知道某个键现在干嘛 → `C-h k` 然后按那个键**(本表若有疑问,以它为准)

---

## 1. 先搞懂:现在是"两层键位叠在一起"

配置在 2026-09-18 大改过一次,现在是:

| 层 | 内容 | 位置 | 谁赢 |
|---|---|---|---|
| **C++ 模式自己的键** | cc-mode 内置(`TAB` 缩进、`M-/` 标记函数…) | 由 `c++-mode` 提供 | 最优先 |
| **你的 VS 层** | VS2022 风格键位(`C-s` 保存、`C-f` 查找、`F12` 定义…) | init.el 第 12 节,`my-vs-keys-mode` | 压过下面的 Casey 层 |
| **Casey 全局层** | Handmade Hero 原版全局键(`M-a` 粘贴、`M-w` 切窗口…) | init.el 第 8、9 节 | 压过 Emacs 默认 |
| **Emacs 默认** | 原厂绑定 | Emacs 自带 | 最后兜底 |

**同一个键在不同文件里不一样(实测)**:

| 键 | 普通缓冲区(`.md`/`.txt`/`*scratch*`) | `.cpp` / `.h` 里 |
|---|---|---|
| `M-/` | `dabbrev-expand`(按当前文件里的词补全) | `c-mark-function`(标记整个函数) |
| `M-c` | `capitalize-word` | `casey-find-corresponding-file`(.h ↔ .cpp 互切) |
| `M-C` | 无绑定 | `casey-find-corresponding-file-other-window` |
| `M-F12` | 无绑定 | `casey-find-corresponding-file-other-window` |
| `TAB` | `indent-for-tab-command` | `c-indent-line-or-region` |
| `RET` | `newline` | `newline-and-indent`(**回车自动缩进**,Casey 的设计) |

> 不确定某个键在你当前文件里是什么 → **`C-h k`**。

---

## 2. 和 VS2022 一致的键(你的 VS 层,全部实测)

| VS2022 里的键 | 在 Emacs 里 | 实测函数 | 说明 |
|---|---|---|---|
| `Ctrl+C` 复制 | **`Ctrl+C`** | 由 `cua-mode` 提供 | **有选中内容时**才是复制 |
| `Ctrl+X` 剪切 | **`Ctrl+X`** | 同上 | **有选中内容时**才是剪切 |
| `Ctrl+V` 粘贴 | **`Ctrl+V`** | `cua-paste` | |
| `Ctrl+Z` 撤销 | **`Ctrl+Z`** | `undo` | |
| `Ctrl+Y` 重做 | **`Ctrl+Y`** | `undo-redo` | |
| `Ctrl+S` 保存 | **`Ctrl+S`** | `save-buffer` | 原 `C-s` 是"搜索",已让位 |
| `Ctrl+Shift+S` 全部保存 | `Ctrl+Shift+S` | `save-some-buffers` | |
| `Ctrl+F` 查找 | **`Ctrl+F`** | `consult-line` | 边打字边预览 |
| `Ctrl+Shift+F` 全局查找 | `Ctrl+Shift+F` | `consult-ripgrep` | 全项目 ripgrep |
| `Ctrl+O` 打开文件 | `Ctrl+O` | `find-file` | 等价 `M-f` / `C-x C-f` |
| `Ctrl+Tab` / `Ctrl+Shift+Tab` | 同左 | `next-buffer` / `previous-buffer` | 也等于 `C-x <right>`/`C-x <left>` |
| `Ctrl+F4` 关闭文件 | 同左 | `kill-buffer` | 会问是否保存 |
| **`F12` 转到定义** | `F12` | `xref-find-definitions` | 靠 clangd/pyright |
| **`Shift+F12` 查找引用** | `Shift+F12` | `xref-find-references` | |
| **`F2` 重命名** | `F2` | `eglot-rename` | 全项目重命名(LSP) |
| `Ctrl+Shift+B` 生成 | `Ctrl+Shift+B` | `recompile` | 另给了 `F5` / `F7` |
| `Ctrl+D` 复制当前行 | **`Ctrl+C` 然后 `D`** | `my-duplicate-line` | `C-d` 仍是"删字符",没让位 |
| `Ctrl+Insert` / `Shift+Delete` / `Shift+Insert` | 同左 | `kill-ring-save` / `kill-region` / `cua-paste` | 经典 Windows 剪贴板,一直可用 |

> ⚠️ **关键理解**:`C-x` 和 `C-c` 在 Emacs 里是**命令前缀**(`C-x C-s` 保存、`C-c p` 项目命令…)。
> `cua-mode` 让它们"**有选区时按 VS 语义复制/剪切,没选区时仍然是前缀**",两边都不耽误。
> 有选区时按了 `C-x`/`C-c` 后 **0.2 秒**内没有后续键,就当作剪切/复制(`cua-prefix-override-inhibit-delay`)。

---

## 3. VS 里有、Emacs 里**不能用**的键(附替代方案)

| VS2022 键 | 这个键在 Emacs 里现在是什么 | 你的替代 |
|---|---|---|
| `Ctrl+H` 替换 | `C-h` = **帮助前缀**(最重要的学习工具) | **`M-o`** 或 **`M-%`** 交互替换 |
| `Ctrl+G` 跳行 | `C-g` = **取消/中断**(救命键) | **`M-g g`** 输入行号跳转 |
| `Ctrl+空格` 智能提示 | `C-SPC` = **设定选区起点**(`cua-set-mark`) | **`M-x completion-at-point`**(见第 6 节) |
| `Ctrl+A` 全选 | `C-a` = **行首**(终端世界的通用约定) | **`C-x h`** 全选;或鼠标三击 |
| `Ctrl+K Ctrl+C` 注释 | `C-k` = **删到行尾** | **`C-x C-;`**(整行注释)或 `M-x comment-region`(选中区域) |
| `Ctrl+K Ctrl+U` 取消注释 | 同上 | 同上(再按一次就是取消) |
| `Ctrl+Shift+空格` 参数信息 | 未占用 | `M-x eldoc-doc-buffer` |
| `Ctrl+-` 返回上一个位置 | `C--` = 数字前缀 | **`M-,`** 跳回;**`M-?`** 查引用 |
| `Ctrl+K Ctrl+D` 格式化文档 | `C-k` 冲突 | **`C-M-\`** 缩进区域;或选中后 `TAB` |
| **`F5` 启动调试** | `F5` = **重新编译** | `M-x gdb` 起调试器(GUD 界面里 `C-h m` 看该模式的键) |
| **`F9` 断点 / `F10` 单步** | `F9` = 第一个错误,`F10` = 上一个错误(**Casey 占了**) | 同上,调试请用 GUD 自己的键 |
| **`F11` 全屏** | `F11` = **下一个错误**(原全屏功能已被顶掉) | `M-x toggle-frame-fullscreen` |

---

## 4. Casey 层:全局新增的键(实测,按用途分组)

这一层是你 2026-09-18 那次改动新加的,以前没有,**不知道会经常踩坑**。

### 4.1 文件 / 缓冲区 / 窗口

| 键 | 实测函数 | 用途 |
|---|---|---|
| `M-f` / `M-F` | `find-file` / `find-file-other-window` | 打开文件(等同 `C-o` / `C-x C-f`) |
| `M-b` / `M-B` | `consult-buffer` / `consult-buffer-other-window` | **切换已打开的文件**(带预览);也等于 `C-x b` |
| `M-k` | `kill-this-buffer` | 关掉当前文件 |
| `M-r` | `revert-buffer` | **从磁盘重读**(⚠️ 不是撤销) |
| `M-p` | `maximize-frame` | 窗口最大化 |
| **`M-w`** | `other-window` | **⚠️ 切到另一个窗口,不是复制!** |
| `Shift+方向键` | windmove | 按方向跳到相邻窗口 |
| `C-c ←` / `C-c →` | `winner-undo` / `winner-redo` | 撤销/重做**窗口布局** |
| `C-x 1` / `2` / `3` / `o` | 只留当前 / 上下分屏 / 左右分屏 / 切窗口 | |

### 4.2 编辑 / 大小写 / 标记

| 键 | 实测函数 | 用途 |
|---|---|---|
| **`M-a`** | `cua-paste` | **⚠️ 粘贴!**(默认 Emacs 里它是"后退一句") |
| `M-z` | `kill-region` | 剪切 |
| `M-q` | `append-as-kill` | 复制并接到上一次剪切的尾巴上 |
| `M-u` | `undo` | 撤销(同 `C-z` / `C-/` / `C-x u`) |
| `M-6` | `upcase-word` | 词 → 大写 |
| `M-^` | `capitalize-word` | 词首字母大写 |
| `M-Backspace`(Alt+退格) | `backward-kill-word` | 往前删一个词 |
| `M-Delete` | `kill-word` | 往后删一个词 |
| `M-SPC` / `C-@` | `cua-set-mark` | 设定选区起点 |
| `M-;` | `cua-exchange-point-and-mark` | **⚠️ 交换光标与标记**(注释功能被它顶掉了) |
| `M-:` | `View-back-to-mark` | 回到上次标记处 |
| `C-x h` | `mark-whole-buffer` | 全选 |
| `C-x C-x` | `cua-exchange-point-and-mark` | 同上 |

### 4.3 替换(改代码时的批量改名)

| 键 | 实测函数 | 用途 |
|---|---|---|
| `M-o` / `M-%` | `query-replace` | 交互式替换(**= VS 的 Ctrl+H**) |
| `M-O` / `F8` | `casey-replace-string` | 直接全部替换,**光标不动** |
| `M-l` | `casey-replace-in-region` | 只在**选区内**替换 |

### 4.4 导航 / 编译错误

| 键 | 实测函数 | 用途 |
|---|---|---|
| `M-j` / `M-g i` | `imenu` | 按名字跳函数/变量 |
| `M-g g` | `consult-goto-line` | 跳行号 |
| **`M-n` / `M-N`** | `next-error` / `previous-error` | 下一个 / 上一个编译错误 |
| `F9` / `F10` / `F11` | `first-error` / `previous-error` / `next-error` | 同上(另有 `C-x \``=下一个错误) |
| `C-↑` / `C-↓` | `previous-blank-line` / `next-blank-line` | 跳到上/下一个**空行**(Casey 的段落级导航) |
| `C-←` / `C-→` / `M-←` / `M-→` | 词移动 | 按词左右移 |
| `Home` / `End` | 行首 / 行尾 | |
| `PgUp` / `PgDn` | `cua-scroll-down` / `cua-scroll-up` | 滚动(**不是翻页**,见第 5 节) |
| `C-PgUp` / `C-PgDn` | `scroll-other-window-down` / `scroll-other-window` | 滚动另一个窗口 |

### 4.5 编译 / 键盘宏

| 键 | 实测函数 | 用途 |
|---|---|---|
| **`M-m`** | `make-without-asking` | **向上递归找 `build.bat` 然后编译**,焦点切到编译输出 |
| `F5` / `F7` / `Ctrl+Shift+B` | `recompile` | 重复上一次编译 |
| `M-[` / `M-]` / `M-'` | `start-kbd-macro` / `end-kbd-macro` / `call-last-kbd-macro` | 录制 / 结束 / 播放键盘宏 |

### 4.6 其他(容易意外的保留项)

| 键 | 实测函数 | 用途 |
|---|---|---|
| `C-x C-;` | `comment-line` | **注释/取消注释整行**(现在唯一的注释键) |
| `C-M-\` | `indent-region` | 缩进选中区域 |
| `M-/` | `dabbrev-expand` | 按当前文件里的词补全(`.cpp` 里是 `c-mark-function`) |
| `M-t` / `M-T` | `load-todo` / `load-log` | Casey 的 TODO / log 文件 |
| `C-,` `C-;` `C->` `C-<` `C-c ,` | multiple-cursors | 多光标:加下一个 / 减 / 全部同词一起加 |
| `C-=` | `er/expand-region` | 按语义逐级扩大选区(词→表达式→块→函数) |
| `C-.` | `embark-act` | 对当前候选/对象执行上下文操作 |
| `C-x g` | `magit-status` | Git 面板(`?` 看全部键) |
| `C-c p` | projectile | 项目命令(`C-c p f` 找文件、`C-c p g` 搜内容) |
| `C-c y` / `C-c Y` | `yas-expand` / `yas-insert-snippet` | 展开 / 挑选代码片段 |
| `C-x C-e` | `eval-last-sexp` | 求值光标前的表达式(改配置时试手) |
| `C-h t` | `help-with-tutorial` | 内置教程(中文,取决于语言环境) |

---

## 5. ⚠️ 被顶掉的 Emacs 默认键(完整清单,实测)

这些键**以前的功能没有了**。按"最容易咬到你"排序:

| 键 | Emacs 默认是 | 你现在按下去是 | 想用回原功能 |
|---|---|---|---|
| **`M-;`** | 注释(`comment-dwim`) | 交换光标与标记 | **`C-x C-;`**(整行) / `M-x comment-region`(区域) |
| **`M-w`** | 复制到 kill-ring | 切到另一个窗口 | **`C-c`**(有选区) / `Ctrl+Insert` / `M-q`(追加复制) |
| **`M-b` / `M-f`** | 按词后退 / 前进 | 切缓冲 / 打开文件 | **`C-←`/`C-→`** 或 `M-←`/`M-→` |
| **`C-v`** | 向下翻页 | 粘贴 | **`PgDn`** 滚动 |
| **`M-v`** | 向上翻页 | 重复替换选区(delete-selection 副作用) | **`PgUp`** 滚动 |
| **`C-f`** | 前进一个字符 | 查找 | **`→`** 方向键 |
| `C-z` | 挂起/最小化窗口 | 撤销 | (一般不需要) |
| `C-y` | 粘贴 | 重做 | **`C-v`** |
| `M-y` | 粘贴历史 | `consult-yank-pop`(仍是粘贴历史,带预览) | 不变,更好用 |
| `M-a` | 后退一句 | 粘贴 | `M-x backward-sentence` |
| `M-u` | 词转大写 | 撤销 | **`M-6`** |
| `M-^` | 合并上一行 | 词首字母大写 | `M-x delete-indentation` |
| `M-q` | 段落重排 | 追加复制 | `M-x fill-paragraph` |
| `M-z` | 删到某字符 | 剪切选区 | `M-x zap-to-char` |
| `M-l` | 词转小写 | 选区内替换 | `M-x downcase-word` |
| `M-k` | 删到句尾 | 关当前文件 | `M-x kill-sentence` |
| `M-r` | 光标移到窗口中间行 | 从磁盘重读文件 | `C-l` 重绘/居中 |
| `M-t` | 转置两个词 | `load-todo` | `M-x transpose-words` |
| `M-j` | 换行并缩进 | `imenu` | `RET`(C++ 里本来就自动缩进) |
| `M-m` | 行首第一非空字符 | 编译 | `M-x back-to-indentation` |
| `M-:` | 求值表达式(`eval-expression`) | 回到标记处 | `C-x C-e` 或 `M-x eval-expression` |
| `M-6` | 数字前缀参数 | 词转大写 | `M-x digit-argument` |
| `M-'` | abbrev 前缀标记 | 播放键盘宏 | `M-x abbrev-prefix-mark` |
| `C-o` | 插入空行 | 打开文件 | `M-x open-line` |
| `C-@` / `C-SPC` | `set-mark-command` | `cua-set-mark`(功能基本等价) | — |
| `C-↑` / `C-↓` | 上/下一段 | 上/下一个空行 | `M-x backward-paragraph` |
| `C-<` / `C->` 等 | 无 | 多光标 | — |
| `C-M-i` | 补全(`complete-symbol`) | **缩进区域** | `M-x completion-at-point`(见第 6 节) |
| `F10` | 打开菜单栏 | 上一个错误 | (菜单栏本来也被关掉了) |
| **`F11`** | 切换全屏 | 下一个错误 | `M-x toggle-frame-fullscreen` |
| `PgUp` / `PgDn` | 上下滚动一屏 | cua 滚动(基本相同) | — |
| `Home` / `End` | 行首 / 行尾(基本相同) | 同左 | — |
| `M-SPC` | `cycle-spacing` | `cua-set-mark` | `M-x cycle-spacing` |
| `M-[` `M-]` | 无 | 键盘宏 | — |
| `<f2>` | 前缀键 | `eglot-rename` | — |

> 同一份配置里还**特意跳过**了 Casey 原版的这几条(所以你的键没被顶):`M-s` 保存、`M-.` 段落重排、`M-g` 跳行、`TAB` 补单词、`C-f` 粘贴、`C-y` 清空、`C-TAB` 重排整块。

---

## 6. 代码补全(corfu + eglot/clangd)

**两层结构**:

1. **语义补全(主力,懂 C++ 语法/类型)**:`eglot` + **`clangd 22.1.6`**(C/C++)、**`pyright`**(Python)
   - 打开 `.cpp/.h/.py` 自动连接,模式行出现 **EGLOT**;
   - `clangd` 启动参数带了 `--query-driver=…cl.exe`,所以 `<windows.h>`、`std::` 都能正常解析。
2. **词级兜底(只在 LSP 给不出候选时)**:`cape-dabbrev`,用当前文件里出现过的词补全(只在编程模式里挂)。

**怎么用(实测)**

| 操作 | 键 | 备注 |
|---|---|---|
| 弹窗自动出现 | 打字即可 | `corfu-auto` 已开 |
| 上下选候选 | `↑` / `↓` | 或 `M-n` / `M-p` —— **仅当弹窗开着时** |
| **接受候选** | **`RET`** | |
| 关闭弹窗 | `C-g` | |
| **强制展开 / 补全** | `M-TAB` 或 `C-M-i` | **仅当弹窗开着时**;弹窗没开时这两个键是**缩进区域** |
| **手动召唤(推荐)** | **`M-x completion-at-point`** | 因为 `C-M-i` 已被 Casey 占成 `indent-region` |
| 参数/文档提示 | 自动显示 | 要完整文档 `M-x eldoc-doc-buffer` |
| 候选的详细信息 | `M-g` / `M-h` | **仅当弹窗开着时**(`corfu-info-location` / `-documentation`) |
| 报错列表 | `M-x flymake-show-buffer-diagnostics` | |
| **`TAB`** | **永远是缩进** | 配置特意把 `TAB` 从 corfu 键表里移除了 |

> ⚠️ **`M-n` / `M-p` 的双重身份**:补全**弹窗开着时**是"选下一个/上一个候选";**弹窗没开时**是 Casey 的 `next-error` / `maximize-frame`。`M-g` 同理(弹窗开着时是"看候选定义"而不是 `M-g` 前缀)。

**没有候选时先查这 4 条**
1. 文件是不是**空的**?空文件没有可补全的词,也没有语义;
2. 模式行有没有 **EGLOT**?没有就 `M-x eglot` 手动连(首次要几秒扫头文件);
3. 是不是在 `*scratch*`、`.md`、`.txt` 里?(那只有很弱的词级补全)
4. 想看 LSP 在干嘛:`M-x eglot-events-buffer`。

---

## 7. 生效方式与回滚

| 目的 | 做法 |
|---|---|
| 让改动生效 | 重启 Emacs,或 `M-x eval-buffer` |
| 只关掉 VS 键位层 | `M-x my-vs-keys-mode` |
| 只关掉 CUA 复制粘贴 | `M-x cua-mode` |
| 临时用纯净 Emacs(查"默认是什么") | `runemacs -q` |
| **整体回滚到加 Casey 层之前** | 用 `init.el.bak-casey-20260918-111046` 覆盖 `init.el` |
| 其他备份 | `init.el.bak-20260917-142233`、`init.el.before-strip-20260917-144337` |
| 配置改坏到起不来 | 把 `init.el` 改名,Emacs 会用默认配置启动 |

> ⚠️ **Casey 层关掉了备份文件和自动保存**(`make-backup-files` = `nil`、`auto-save-default` = `nil`),
> 所以 `.emacs.d\backups\` 和 `auto-save\` 两个目录**现在不再被使用**;工程目录里也不会再冒出 `~` 和 `#` 文件。

---

## 8. 本机环境速查

| 项目 | 值 |
|---|---|
| Emacs | **30.2**,`C:\THOTH\RuanJian\emacs-30.2`(桌面快捷方式「Emacs」) |
| 配置 | `C:\Users\thoth\AppData\Roaming\.emacs.d\init.el`(967 行,12 节) |
| Casey 原件副本 | `C:\Users\thoth\Downloads\casey-emacs-original.el`(601 行)、`casey-emacs-keybindings.txt` |
| clangd | `C:\THOTH\RuanJian\clangd-22.1.6\clangd_22.1.6\bin`(C/C++ 补全与跳转) |
| pyright | `C:\Users\thoth\AppData\Roaming\npm`(Python 补全) |
| `M-m` 编译的目标 | 向上递归找到的第一个 **`build.bat`**(`casey-makescript`) |
| `grep` 命令 | `findstr -s -n -i -l`(Windows 下 Casey 的替代方案) |
| 启动时窗口 | **自动左右分成两个窗口**(`split-window-horizontally`),且禁止自动分窗 |
| 外观 | 背景 `#161616`、前景 `burlywood3`、光标 `#40FF40`、字体 `Consolas-11.5`;菜单栏关闭 |
| ⚠️ 别改的文件 | `C:\Users\thoth\AppData\Local\clangd\config.yaml`(**Qt Creator 生成的**,里面 `UnusedIncludes: Strict` 会给很多文件刷黄色波浪线) |

---

## 9. 自己核对键位(推荐常做)

| 想知道 | 做法 |
|---|---|
| 某个键现在是什么 | **`C-h k`** 然后按那个键(最权威) |
| 当前缓冲区所有绑定 | **`C-h b`** |
| 某个命令挂在哪个键 | `M-x where-is` |
| 某个前缀键下面有什么 | 按下前缀后停住,或 `which-key` 会自动弹出来 |

**本次生成这张表的实测方法**(想重跑就用它):

```powershell
# 用真实 Emacs 加载你的 init.el,把每个键的实际绑定打印出来
& "C:\THOTH\RuanJian\emacs-30.2\bin\emacs.exe" --batch -Q -l "你的脚本.el"
```

脚本里针对每个键调用 `(key-binding (kbd "M-;"))`,再用 `where-is-internal` 反查命令;
把 `-Q`(不加载 init)和加载 init 的两次结果相减,就得到第 5 节那张"被顶掉的默认键"清单。
