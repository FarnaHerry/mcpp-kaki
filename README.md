# mcpp-kaki

修仙题材的 2D 动作游戏（类银河恶魔城 Metroidvania），使用 **Godot 4.6 + C++ GDExtension** 编写。

A xianxia (cultivation fantasy) 2D action game in the Metroidvania style, built with **Godot 4.6 + C++ GDExtension**.

所有游戏逻辑用 C++ 编写（不使用 GDScript），构建系统用 **mcpp**（现代 C++23 模块化构建工具）。全项目（含 godot-cpp 官方绑定）统一使用 **C++23** 标准编译。

---

## ✨ 特性 / Features

- **修仙体系**：13 境界、修为经验、突破（机缘事件/心魔劫/三灾渡劫）、功法修炼、炼丹、法宝温养、宗门拜师、威压灵压
- **2D 动作**：8 状态玩家（Idle/Run/Jump/Fall/WallCling/Dash/Attack/Fly）、连击、飞行（筑基借飞剑 / 金丹自主）、二段跳、墙面跳
- **技能系统**：武技/法术/神通/仙法/被动统一 Skill 管线，元素伤害（金木水火土雷）+ 五行克制
- **能力门控探索**：炼气纳戒、筑基御剑飞行、化神缩地成寸等境界解锁
- **四大部洲世界**：东胜神洲（花果山/水帘洞/东海之滨）、云海强渡旅行、五区地图（竹台跳跃/墙跳/飞行沟壑）
- **随身洞天**：炼虚期开辟小世界，O 键随时进出；灵田种植（种一收二，现实时间生长），聚灵阵/扩张规划见 design/dongtian.md
- **数据驱动**：物品/技能/功法/宗门/掉落/配方/洲全部外置为 `data/*.json`
- **模块化 C++**：自己的代码用 C++ 模块（`import`）组织，godot-cpp 绑定在模块接口中以 `import godot_cpp;` 接入（普通 .cpp 仍走头文件），全项目统一 C++23

## 🛠 技术栈 / Tech Stack

| 组件 | 说明 |
|---|---|
| Godot | 4.6 (config_version=5), gl_compatibility (2D) |
| 分辨率 | 480×270 内部 → 1920×1080 canvas_items stretch, Nearest pixel art |
| 语言 | C++23（全项目统一，含 godot-cpp 官方绑定）；少量 GDScript 仅做场景装配 |
| 构建 | **mcpp** (C++23 模块化构建工具) |
| 绑定 | godot-cpp —— mcpp 官方包 `compat:godot-cpp`（预生成绑定，随 `mcpp build` 自动拉取） |

## 📁 目录结构 / Structure

```
src/               # C++ 源码（模块化）
  utils.cppm       # module mcpp_kaki.utils —— SignalBus/Localization
  combat.cppm      # module mcpp_kaki.combat —— 伤害/连击/技能
  inventory.cppm   # module mcpp_kaki.inventory —— 物品/背包
  cultivation.cppm # module mcpp_kaki.cultivation —— 修仙系统
  nodes.cppm       # module mcpp_kaki.nodes —— UI 节点
  core.cppm        # module mcpp_kaki.core —— 数据/存档/洲
  register_types.cpp # GDExtension 入口
scenes/            # .tscn 场景
data/              # 外置游戏数据（JSON）
scripts/           # GDScript 装配 + 构建脚本
bin/               # 编译产物（gitignored）
```

> **模块化说明**：约 70% 的类已用 C++ 模块（`import mcpp_kaki.*`）组织，6 个模块接口（.cppm）经 mcpp 模块包 `godotengine:godot-cpp-m` 以 `import godot_cpp;` 接入引擎绑定（宏走 `<godot-cpp-m/macros.h>`，HashMap/HashSet 保持文本包含）。绑定 godot 内置类指针（`Node*`/`Node2D*`/`Object*`）的节点类（Player/Enemy 等）保持头文件——这是 C++ 模块与 godot-cpp 绑定模板的已知限制。

## 📦 环境要求 / Requirements

- **Godot 4.6**（`godot` 在 PATH 中）
- **mcpp** 构建工具（`mcpp --version`），含 LLVM 工具链（`mcpp toolchain install llvm`）

> godot-cpp 绑定由 mcpp 包 `compat:godot-cpp@10.0.0-rc1`（Godot 4.6）提供，首次 `mcpp build` 自动下载并编译，无需 Python / SCons / git 子模块。

## 🔨 编译 / Build

### 方式一：脚本（推荐）

```bash
# 一键构建：mcpp 编译（自动拉取 godot-cpp 依赖）→ 复制 .so 到 bin/
./scripts/build_mcpp.sh

# 产物：
#   bin/libmcpp-kaki.linux.editor.x86_64.so
#   bin/libmcpp-kaki.linux.template_debug.x86_64.so
```

### 方式二：手动分步

```bash
# 1. 用 mcpp 编译 GDExtension（godot-cpp 依赖首次自动下载，之后走全局缓存）
mcpp build

# 2. 复制产物到 bin/（Godot 从这里加载）
cp target/x86_64-linux-gnu/*/bin/libmcpp-kaki.so \
   bin/libmcpp-kaki.linux.template_debug.x86_64.so
cp target/x86_64-linux-gnu/*/bin/libmcpp-kaki.so \
   bin/libmcpp-kaki.linux.editor.x86_64.so
```

**mcpp 环境准备**（首次，若未装）：

```bash
# 安装 mcpp 工具链（llvm）
mcpp toolchain install llvm
```

## ▶️ 运行 / Run

```bash
godot
```

> **注意**：需要图形显示。Headless 模式（`godot --headless`）**不会加载 GDExtension**。

## 🎮 操作 / Controls

| 按键 | 功能 |
|---|---|
| 方向键 | 移动 |
| X | 普攻 / 交互 / 菜单确认（交互优先） |
| C | 跳跃（空中再按 = 飞行） |
| Z | 冲刺 |
| V | 威压（慑服低阶敌人） |
| R | 灵压（法术伤害/镇杀低阶） |
| O | 进出洞天（炼虚解锁，安全状态可用） |
| Space | 确认副键 |
| I | 背包 |
| Q | 修炼突破 |
| ESC | 多页菜单（背包/能力/功法/技能/法宝/宗门/云游/炼丹/设置），菜单内 ←/→ 或 Q/E 翻页 |
| A / S | 武技槽 |
| D / F | 法术槽 |
| T | 神通槽 |
| Y | 仙法槽 |
| B | 切法宝页 |
| 1~6 | 消耗品快捷栏 |
| F3 | 遥测面板 |
| F4 | 游戏 HUD |
| F5 | 突破无经验门槛（调试） |
| F6 | 读档 |

## 📜 License

MIT
