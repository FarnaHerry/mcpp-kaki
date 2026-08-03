# Session 002 — mcpp 构建迁移 + C++20 模块化重构 + 开源准备

**日期**: 2026-08-04  
**分支**: main  
**起始提交**: 56c4204  
**结束提交**: 52293d4  
**本会话提交**: b32a90c / 9343d89 / 52293d4

## 完成内容

### 1. 构建系统：CMake → mcpp

探索并确认 **mcpp** 是 mcpp-community 的现代 C++23 构建工具（非老预处理器），并完成全量迁移：

- **POC 验证**：godot-cpp 完全可以用 mcpp 编译（手动生成绑定 + mcpp.toml 配置），生成的 `.so` 能被 Godot 4.6 加载运行
- **移除 CMake**：删除 CMakeLists.txt，`mcpp.toml` 成为唯一构建配置
- **新增脚本**：
  - `scripts/build_mcpp.sh` — 一键构建（生成绑定 → mcpp 编译 → 复制 .so）
  - `scripts/generate_godot_bindings.py` — 按 Godot 4.6 API 生成绑定到 `mcpp-gen/`
- **统一 C++23**：mcpp 用 `-std=c++23` 编译所有源码（含 godot-cpp 官方绑定），不再区分 godot-cpp 的 C++17

关键结论：godot-cpp 对 mcpp 只是一个源码目录（`godot-cpp/src/**`），由 `[modules].sources` 直接编入同一个 `.so`，不调用其自带构建系统。

### 2. C++20 模块化重构（约 70% 类）

POC 验证 **GDCLASS + C++20 模块 + mcpp + Godot 4.6** 可行后，分块推进：

| 模块 | 内容 |
|---|---|
| `mcpp_kaki.utils` | SignalBus, Localization |
| `mcpp_kaki.combat` | 伤害类型/结算、连击、HitBox/HurtBox/Projectile、SkillSystem |
| `mcpp_kaki.inventory` | Item, Inventory, ItemDatabase |
| `mcpp_kaki.cultivation` | 11 个修仙系统（境界/功法/Buff/宗门/炼丹/法宝/突破/三灾等） |
| `mcpp_kaki.nodes` | GameHUD, GameMenu, DamageNumbers, InventoryPanel |
| `mcpp_kaki.core` | DataLoader, SaveSystem, ContinentManager |

**沉淀的模块化教训**（踩坑记录）：
- 模块实现单元的 `import` 必须在 `module mcpp_kaki.X;` 之后、`namespace godot {` 之前——放错位置会引发 godot-cpp 枚举跨模块重复定义冲突
- 外部类型（Player 等）的前向声明放模块接口的 **global fragment**，放模块作用域会产生模块链接冲突
- 头文件里的 `import` 必须在全局作用域（namespace 外）
- `VARIANT_ENUM_CAST` 必须移到模块接口，否则 `GetTypeInfo<...Enum>` 未定义
- 已模块化类型的前置声明（`class HitBox;`）必须删除

**硬限制（无法绕过）**：
- **绑定 godot 内置类指针**（`Node*`/`Node2D*`/`Object*`）的方法在模块实现单元触发 `make_property_info` ADL 失败 → 这类类保持头文件（Player, Enemy, CameraRoom2D, Portal, ItemPickup, HerbNode, TelemetryPanel, GameManager, DropSystem）
- **自定义类指针**（`Player*`）绑定在模块里可行（AlchemySystem 验证）
- **mcpp 拒绝循环模块依赖**（nodes ↔ cultivation），只能靠保持 Player/Enemy 为头文件打破环

### 3. 开源准备

- **敏感数据检查**：密钥/token/密码/邮箱/IP/绝对路径/git 历史泄漏全部干净；`compile_commands.json` 补 gitignore
- **改名 cpp-kaki → mcpp-kaki**：目录、`.gdextension`、入口符号 `mcpp_kaki_library_init`、模块名、`.so` 名、project 名全部改名；清理 Godot 扩展缓存（`.godot/extension_list.cfg` + 旧 `.uid`）
- **文档**：新增 `README.md`（脚本 + 手动编译指令、运行、操作表、模块化说明）、`LICENSE`（MIT）
- **发布**：推送到 `github.com:FarnaHerry/mcpp-kaki.git` (main)

### 4. 删除的文件

- `CMakeLists.txt`（CMake 构建废弃）
- `docs/module-refactor.md`（重构过程跟踪，任务完成后按约定删除）

## 当前项目状态

```
/home/farna/dev/godot/mcpp-kaki
├── README.md / LICENSE / mcpp.toml / mcpp_kaki.gdextension
├── scripts/        # build_mcpp.sh + generate_godot_bindings.py
├── src/            # 6 个模块化 C++ 源码（统一 C++23）
├── scenes/ data/ design/
└── godot-cpp/      # 官方子模块（C++23 编译）

构建: ./scripts/build_mcpp.sh
运行: godot
仓库: github.com/FarnaHerry/mcpp-kaki
```

## 待办 / 后续方向

- `git push`（本会话推送由用户手动完成，环境内 SSH 认证失败）
- 若需要：将模块化边界结论整理成给 mcpp / godot-cpp 社区的反馈文档
