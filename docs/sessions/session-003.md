# Session 003 — godot-cpp 切换 mcpp 包管理 + 模块形态 import godot_cpp

**日期**: 2026-08-04
**分支**: main（经 feat/mcpp-godot-cpp 快进合并）
**起始提交**: d17e1eb
**结束提交**: ecb7104
**本会话提交**: 7de88de / 537fe5b / ecb7104

## 完成内容

### 1. godot-cpp 子模块 → mcpp 包管理（compat:godot-cpp）

mcpp 官方注册表已提供 Godot 支持（`mcpp search godot` 可见），本项目从"子模块 + 自研 Python 生成脚本"切换为纯包管理：

- **`mcpp add compat:godot-cpp@10.0.0-rc1`**：Godot 4.6 预生成绑定（作者离线跑上游 `binding_generator.py` 后打包不可变镜像 tarball），消费侧零 Python/SCons
- **移除**：`godot-cpp/` 子模块、`mcpp-gen/`、`scripts/generate_godot_bindings.py`、`extension_api.json`、`.gitmodules` 条目
- **`mcpp.toml`**：删 `godot-cpp/src`、`mcpp-gen/gen/src` 两个 sources glob 及对应 include_dirs；版本由 `mcpp.lock` 锁定
- **`build_mcpp.sh`**：删绑定生成步骤，剩 `mcpp build` + 复制 .so 两步

**收益**：godot-cpp 1075 个 TU 进 mcpp 全局缓存，clean 重编从几分钟降到 **4.4s**；仓库甩掉子模块和 Python 依赖。

**踩坑**：
- `ability_manager.cpp`（模块实现单元）报 `HashSet` 不可见——原来靠 rc1+85 头文件的传递包含侥幸通过，rc1 头文件不再传递；全局模块片段补 `#include <godot_cpp/templates/hash_set.hpp>`（模块实现单元本就需自行包含，IWYU）
- 包不定义 `DEBUG_ENABLED`（项目 cflags 有）：只要不调仅调试方法（`DEBUG_METHODS_ENABLED` 声明的）就无影响，实测零问题
- 包统一定义 `GDEXTENSION` + `TYPED_METHOD_BIND`（feature 传播给消费方，一致性由 mcpp 保证）
- 版本从子模块 rc1+85 回退到 rc1：GDExtension 同 minor 向后兼容，实测 Godot 4.6.3 无影响

### 2. 顺带修复：GameMenu ←/→ 翻页回归（537fe5b）

跑 `test_travel` 发现 17 项 FAIL，初判为测试腐化（宗门页插入后页码后移），深挖后确认**是真 bug**：`b32a90c` 模块化重构时把 `_process` 里的 ←/→ 动作轮询翻页删掉了，换成 `_input` 的 Q/E 物理键，但各页 hint 仍写「←/→ 切换页」。

修复：`_process` 恢复 ←/→ 翻页（设置页音量/语言行的 ←/→ 调节优先），Q/E 作为额外快捷键保留。修完 `test_travel` 26 项 ALL PASS。

教训：集成测试的 FAIL 不要急于归因为"测试过期"，先验证游戏行为是否符合 UI 文案。

### 3. 模块形态：`import godot_cpp`（godotengine:godot-cpp-m）

- **`mcpp add godotengine:godot-cpp-m@10.0.0-rc1`**：C++23 模块层，依赖共享同一 compat.godot-cpp 构建（不重复编译）
- **6 个 `.cppm` 模块接口**全部切换：引擎类/变体头文件 → `import godot_cpp;`
  - 宏（GDCLASS/GDREGISTER/D_METHOD/memnew/ERR_*）→ `#include <godot-cpp-m/macros.h>`（模块带不动预处理器产物）
  - HashMap/HashSet/Vector 模块不重导出（inline 体触及 static+匿名 union，导出 TU-local 类型是硬错误）→ `templates/*.hpp` 保持文本包含
- 模块实现单元与普通 `.cpp`（52 个）不动：godot-cpp 头文件在模块的全局模块片段里，include 与 import 指向同一批全局模块实体，混用 well-formed

**模块包实现原理**（`godot_cpp.cppm`，工具生成）：GMF 文本包含全部 godot-cpp 头文件（约 1100 行 include）→ `export module godot_cpp;` → `export namespace godot { using ::godot::X; ... }`（约 1700+ 个 using 声明把全局实体名字摆上模块导出面）。

**验证**：全量构建零错误一次通过；Godot 4.6.3 启动正常；`test_combat / skills / travel / breakthrough / sect / passives` 全部 0 FAIL。

## 当前项目状态

```
/home/farna/dev/godot/mcpp-kaki
├── README.md / LICENSE / mcpp.toml / mcpp.lock / mcpp_kaki.gdextension
├── scripts/        # build_mcpp.sh（无 Python 步骤）
├── src/            # 6 个模块接口 import godot_cpp + 52 个头文件体系 .cpp（统一 C++23）
├── scenes/ data/ design/
└── （无子模块）    # godot-cpp 由 mcpp 包提供：compat:godot-cpp + godotengine:godot-cpp-m

依赖: compat:godot-cpp@10.0.0-rc1（Godot 4.6 预生成绑定，静态库）
      godotengine:godot-cpp-m@10.0.0-rc1（import godot_cpp 模块层）
构建: ./scripts/build_mcpp.sh   # clean 重编约 4.4s（依赖全局缓存）
运行: godot
仓库: github.com/FarnaHerry/mcpp-kaki (main，已推送)
```

## 待办 / 后续方向

- Godot 升级时的版本路径：`mcpp.toml` 改包版本号 → `mcpp update`（若所需 Godot 版本尚无对应包，需向 mcpp 社区提 issue）
- 可考虑把"Godot + mcpp 全模块 GDExtension"实践整理成示例/反馈给 mcpp 社区
- 模块化剩余边界：Player/Enemy 等绑 godot 内置类指针的类仍保持头文件（`make_property_info` ADL 硬限制，见 session-002）
