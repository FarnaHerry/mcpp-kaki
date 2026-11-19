# 美术素材管线 — 白衣仙子 + 全敌人 + 世界物品/灵植/NPC/设施/地形/战斗 FX

**日期**: 2026-08-13（规划） / 2026-10-17（玩家 v1 / 敌人 v1 落地） / 2026-11-20（玩家 v3 + 世界素材扩展）
**状态**: 玩家与敌人全套动作已接线；49 物品（含 GridList 格内图标）、9 灵植、8 类 NPC、10 类设施、12 种地形、8 类弹体、8 类技能 FX 与 12 套完整背景已落地。

## 落地实况（v2：敌人动作素材，4 原型 × 5 状态 = 80 帧）

### 规格（与玩家套的关键差异）

| 项 | 取值 | 为什么 |
|---|---|---|
| 帧格 | **统一 48×48** | 四原型体型差极大（飞行 18px / Boss 34px）；统一帧格让运行期只有一张图集、一套锚点规则，空白像素不花钱 |
| 锚点 | 每原型一个（melee/ranged 41、flying 37、boss 43）= 描边底 | 引擎按 `定义尺寸.y` 反推 offset |
| 配色 | **中性灰阶**（hi/body/body_d/plate/dark/eye） | 运行期按 `Enemy.get_def_color()` 染色：0.45 中性底 + 0.55×定义色 → **68 个敌人定义共用 4 套素材仍有色相区分**，且明暗不被压死 |
| 原型判定 | `boss > flying > ranged > melee`（读 `is_boss/is_flying/is_ranged` 属性） | 生成器不需要认识任何具体敌人 id；`data/enemies.json` 加怪自动落到某原型 |
| 状态 | idle/walk/attack/hurt/death（80 帧） | Enemy FSM 8 态映射到 5 态：patrol/chase/flee→walk，boss_special→attack |
| 图集 | 240×960（5 列 × 20 行，每 (原型,状态) 一行） | 与玩家套同一「每状态一行」契约 |

### 引擎接线（`scripts/art/enemy_visual.gd`）——三处与玩家层不同，都是被现有代码形状逼出来的

1. **素材挂在 `Polygon2D` 子节点下**（不是敌人根下）：`Polygon2D.scale` 是本项目事实上的体型权威——
   `Enemy::make_elite` 给它 ×1.15/×1.3，12 处场景脚本直接给它 ×1.3~×2.4；敌人根 `modulate`
   （精英词缀染/威压压灰）也经此继承。零调用点改动即保住全部既有表现。
2. **占位方块用「清空 polygon」而不是 `visible=false` 停画**：Godot 的 visible 会被子节点继承，
   `visible=false` 会把素材一起藏掉。清空多边形保留节点/scale/color 语义（`test_elite` 的
   ×1.15 断言、心魔镜像读色都不受影响）。
3. **按定义尺寸等比缩放到碰撞块**：`素材身高(sil_height) → 定义尺寸.y`，视觉大小与碰撞块一致
   （否则技能/攻击看起来「打空」）。

C++ 侧新增两个绑定：`Enemy.get_state_name()`（态名，脚本只做映射不重算）与
`Enemy.get_facing_direction()`（朝向，输入/贴墙/追击都在 C++ 侧改，脚本重算必脱节）。

### 生成/校验工具链（生成器无关）

`validate.py` 与 `ascii_view.py` 现在消费统一的 `iter_assets()` 契约
（`[{group, frame, anchor, air_ok, states}]`），玩家套与敌人套共用同一套 7 类断言与 ASCII 目视。

```bash
python3 tools/pixelart/enemy_sprites.py                                   # 生成 80 帧 + 图集 + manifest
python3 tools/pixelart/validate.py --module enemy_sprites                 # 校验（退出码非 0 = 不合格）
python3 tools/pixelart/ascii_view.py --module enemy_sprites --group boss --state idle
```

### 敌人套踩过的坑（与玩家套同源 + 新增）

- **接地态不许正向 bob**：走/待机必须每帧至少一条腿 `dy=0`，否则脚离锚点（校验器锚点断言把关）。
- **1px 细节会被描边吃掉**：角/爪/牙/背刺一律 ≥2px，否则成品里变成黑线。
- **帧边裁剪**：Boss 头/牙、飞行尾摆、爪前伸都曾越界；统一「夹住前伸距离」而不是缩整体。
- **死亡不能靠一直下沉**：Boss 基线从 44 抬到 42 留出倒地空间，改以「塌+腿外张」表达。
- **`bool(null)` 会抛异常**：读 `is_boss` 等属性要用类型安全取值（非 Enemy 节点没有这些属性）。
- **幂等守卫要找对层级**：素材挂在占位下，`get_node_or_null` 必须查占位而非敌人根（曾导致重复挂载）。
- **AtlasTexture 不能当图集再嵌套**：敌人层 `_region()` 曾用当前 `texture` 作 `atlas`，首个状态后 `texture` 已是 AtlasTexture，切新状态便采空；现与玩家层一致，用独立 `_sheet` 永远引用原图集。`test_enemy_visual.gd` 已覆盖 68 个定义挂载/可见性及 4 原型全状态全帧图集引用。

## 世界素材扩展（2026-11-20）

| 类别 | 覆盖 | 生成器 / 运行期接线 |
|---|---|---|
| 物品 | **49 个独立 16×16 图标**；品级 modulate 与 GradeBeam 保留 | `item_sprites.py` → `item_visual.gd`；在 `ItemPickup._create_visual()` 挂到 `PickupVisual` 下，原生掉落也覆盖 |
| 灵植 | **9 种 ×（生长 4 + 枯萎 1）=45 帧** | `herb_sprites.py` → `herb_visual.gd`；轮询 `is_harvested()` 切枯萎并显式降色 |
| NPC | **8 身份 × 4 帧=32 帧**：村民/长老/客栈/商人/仙人/僧侣/判官/帝王 | `npc_sprites.py` → `npc_visual.gd`；覆盖 TownNpc、ShopKeeper、NarrativeNode，保留名字/气泡/overlay |
| 设施 | **10 类 / 44 帧**：门/箱/云游阵/炼体/打坐/丹炉/扩张碑/阵眼/场景门/条件门 | `prop_sprites.py` → 通用 `prop_visual.gd`；云游阵 inactive/active 自动切换，定制门可 opt-out |
| 地形 | **12 种 16×16 可平铺 tile** | `terrain_sprites.py` → `terrain_visual.gd`；保留 Polygon2D 几何，设置独立 tile + UV + repeat |
| 弹体 | **物理 + 金木水火土雷风，8×4=32 帧** | `projectile_sprites.py` → `projectile_visual.gd`；根 rotation 导向，元素绑定已注册 |
| 普攻/技能 FX | **8 类 / 40 帧**：剑气、旋风、升龙、突进、闪现、增益、护盾、剑扇 | `fx_sprites.py` → `fx_visual.gd`（普攻）+ `skill_fx_visual.gd`（SkillSystem EffectKind），末帧自动回收 |
| 场景背景 | **12 套 480×270**：洲/云海/地府/洞天/洞穴/海底/火窟/宫殿 | `background_sprites.py` → `background_visual.gd`；CanvasLayer -100，覆盖 WC.setup、洞天及 Portal 进房 |
| UI 物品图标 | 49 物品图标复用到背包/仓库/商店格子 | GridList 条目新增 `item_id`，独立 PNG + TextureRect；非物品/未知 id 保持纯文字 |

统一约束：素材缺失/未知 id 保留原占位；只清空指定 `Polygon2D.polygon`，不隐藏含子节点的父节点；
NPC/TeleportArray 重建先 `remove_child()` 再 `queue_free()`，避免新名称节点被自动改成 `@Label@…`。
**地形平铺不能用 AtlasTexture**：repeat 会采到相邻图块；运行期加载 manifest 指定的独立 `tile_*.png`，图集仅用于预览。

```bash
python3 tools/pixelart/item_sprites.py
python3 tools/pixelart/herb_sprites.py
python3 tools/pixelart/npc_sprites.py
python3 tools/pixelart/prop_sprites.py
python3 tools/pixelart/terrain_sprites.py
python3 tools/pixelart/projectile_sprites.py
python3 tools/pixelart/fx_sprites.py
python3 tools/pixelart/background_sprites.py
godot --headless --editor --import
```

对应 harness：`test_world_art.gd`（9 断言）、`test_prop_visual.gd`（8）、
`test_terrain_visual.gd`（4）、`test_combat_art.gd`（5）、`test_grid_item_icons.gd`（3）、
`test_skill_fx.gd`（3）、`test_background_visual.gd`（4）。

## 落地实况（v3：白衣仙子主角 9 状态动作帧）

**主角造型（2026-11-20）**：墨蓝垂腰长发/半挽小髻、淡金玉簪、青色大眼、雪白交领广袖仙衣、层叠长裙与淡青飘带；常态背剑、攻击持剑、飞行御剑。仅重画 `player_sprites.py` 部件与调色板，FSM、碰撞、帧数、锚点和运行期接线均不变。

### 风格决策（§0 的决策点，已选）

**像素风**，与现状一致：Nearest 过滤 + 480×270 内部画布 + `content_scale_size` 不变。
素材**按目标尺寸直接绘制**（真像素 1:1），不走「大图缩小」。

| 项 | 取值 |
|---|---|
| 帧格 | **32×32**（角色本体约 16×28，与 Player 占位方块同footprint） |
| 锚点 | 帧内 **(16, 29)** = 描边底（脚底）；引擎侧 `offset=(0,1)` 对齐占位方块底边 |
| 朝向 | 只画**朝右**，朝左由引擎 `flip_h` |
| 配色 | 白衣仙子语义调色板（雪白/月白仙衣、墨蓝发、淡青衣缘与玉饰、青瞳、冷色剑刃），描边色 + 明暗派生由收尾统一生成 |
| 状态 | Player FSM 全部 9 态：idle run jump fall wall_cling dash attack fly meditate（29 帧） |

### 生成链路（确定性、可复现）

```
python3 tools/pixelart/player_sprites.py     # 生成素材（29 帧 + 图集 + manifest + 预览）
python3 tools/pixelart/validate.py           # 素材校验（退出码非 0 = 不合格）
python3 tools/pixelart/ascii_view.py idle    # 无图像输入时的目视校验（ASCII 像素视图）
```

- `tools/pixelart/pixel_lib.py` — 像素画布（矩形/直线/椭圆/渐变条/多边形）+ 统一收尾
  `finish()`：右上暗（光自左上）/上缘亮/下缘沉 + 描边；`seal_holes()` 封细缝。
- `tools/pixelart/player_sprites.py` — 姿势表（每态若干 `P(...)` 帧参数）+ 部件画法
  （垂腰长发/半挽发髻/玉簪/动漫眼/交领广袖/层叠仙裙/背剑/持剑/飘带）+ 输出。
- **确定性红线**：同参数必须产出 bit-identical PNG（无随机、无时间戳）——素材入库后任何人
  重跑生成器 `git diff` 必须为空；`validate.py` 有确定性断言把关。
- **不做紧凑图集**：图集「每状态一行」（列=帧序），manifest `row` 才能与行号一一对应。

### 产物与契约

```
assets/textures/player/player_<state>_<NN>.png   单帧源（29 张，可单独替换）
assets/textures/player/player_sheet.png          运行期图集 192×288（6 列 × 9 行）
assets/textures/player/manifest.json             帧表：frame_size/anchor/sheet_cols/states{row,count,fps,loop}
docs/art/player_preview.png                      ×6 目视大图（目录含 .gdignore，不入引擎）
```

`player_visual.gd` 只读 manifest，**不认识生成器**：手工/AI 素材只要给出同构 manifest
（同帧格、同锚点、同状态名）即可无缝替换，无需改引擎代码。

### 引擎接线

- `scripts/art/player_visual.gd`（`Sprite2D`，节点名 `PlayerVisual`）：读 manifest + 图集，
  按 `Player.get_state_name()` 逐帧播放、`get_facing_direction()` 决定 `flip_h`；
  `run` 帧率随水平速度同比缩放（境界提速后步频不脱节）；非循环态（attack/jump）停在末帧。
- 挂载点：`WorldCommon.setup()`（`load(...).call("attach", player)`），**幂等**。
- **回退**：manifest/图集缺失 → `attach()` 返回 `null`，程序化 `Polygon2D` 占位原样保留
  （占位节点只隐藏不删除——`Polygon2D` 仍被测试与心魔镜像取色读取）。
- C++ 侧新增 `Player.get_facing_direction()`：朝向是 C++ 事实，脚本不重算（输入反转/贴墙/击退
  都会改朝向，脚本侧重算必然脱节）。
- 状态名 = manifest 动画名；素材缺某态 → 兜底 `idle`（日后 FSM 加态不会黑屏）。

### 导入设置（§2）

- 项目级默认（`project.godot [importer_defaults]`）：`compress/mode=0`（无损）、
  `mipmaps/generate=false`、`detect_3d/compress_to=0`（关 Detect 3D）。
- 单个 `.import` 文件同样已按此设定并入库（换机器重导入不会退回默认有损/带 mipmap）。
- 过滤走项目设置 `rendering/textures/canvas_textures/default_texture_filter=0`（Nearest）。
- **改图后必须重导入**：直接执行 `godot` 不会扫描变更 PNG，会继续读取 `.godot/imported` 旧缓存。生成素材后运行 `godot --headless --editor --import`，再启动游戏或 harness。

### 校验器把关的 7 类约束（`tools/pixelart/validate.py`）

1. **无细缝**：封闭腔中长宽任一方向 < 2px 的块必须已被 `seal_holes()` 封掉
   （细缝在成品里就是一条黑线/黑点）；≥2×2 的腔是**正常肢体间隙**（手臂与躯干之间），保留。
2. **不贴画布边**：素材四边留 1px 描边余量，否则描边被裁。
3. **锚点精确**：接地态（idle/run/dash/attack/meditate）描边底必须落在 y=29；
   腾空态（jump/fall/fly/wall_cling）豁免——否则引擎里会「悬空」或「陷地」。
4. **帧间有位移**：相邻帧像素必须变化（防「看着在动其实静止」）。
5. **首末帧不重复**：循环态不允许首末帧完全相同（防帧数虚高）。
6. **确定性**：同参数重复渲染逐字节一致。
7. **用色统计**：报告每态用色数（当前 47~109，明暗派生所致）。

### 已知限制 / 下一轮可做

- **背景精修**：12 套完整远景已覆盖洲/房间；后续可增加多层视差、天气与建筑动画。
- **环境/渡劫 FX**：技能 EffectKind 已有统一短帧管线；三灾、岩浆、极寒、弱水等环境表现仍可逐区增强。
- **调色板量化**：当前明暗为连续乘法派生，用色数偏多；若要严格像素画色阶，
  可改为「基础色 → 3 档色阶」吸附（`pixel_lib` 已集中收尾，改一处即可）。
- **图集打包**：单帧 PNG 与图集并存；帧多了再考虑只留图集。地形是例外——运行期必须用独立 tile 才能安全 repeat。

---

## 以下为原始规划（保留决策记录）

### §0 决策点先定：风格

- **像素风** → 保持现状：Nearest 滤镜 + 480×270 内部画布 + 整数倍缩放。素材按真像素 1:1 导出。
- **高清手绘风** → 改配置：滤镜 Nearest→Linear、`content_scale_size` 提高（960×540 起）。

**已选像素风**（见上）；二选一，不混。

### §1 格式标准

统一 **PNG**（无损、带 alpha、Godot 直接导入）。角色/敌人/道具精灵（需透明）→ PNG；
动画帧 → PNG 单帧（量大再只留图集）；大背景 → PNG 或无损 WebP；UI/图标 → PNG。

**明确不用**：❌ JPEG（有损+无透明）❌ SVG（本质仍光栅化，绕一圈不可控）❌ DDS/KTX2（发布期优化，不是制作源格式）。

### §2 Godot 导入设置（像素风关键，勿用默认）

Filter = Nearest、关 Mipmaps、压缩 = Lossless、不缩放、禁用 Detect 3D —— **已落地为项目默认**（见上）。

### §3 工具分工

| 环节 | 工具 | 说明 |
|---|---|---|
| 代码/配置/数据 | DeepSeek（本仓库） | 现役 |
| **程序化像素素材** | `tools/pixelart/*`（Python + PIL/numpy） | **v1 落地**：确定性、可复现、无外部依赖额度 |
| 审图/一致性校验 | 多模态模型 / `ascii_view.py` | ASCII 视图可在无图像输入时校验形状 |
| 手工/AI 精修 | Aseprite / 像素专用链路 | 产出同构 manifest 即可直接替换 |

### §4 生成工作流注意

直接按目标像素尺寸出图（别先生成 512×512 再缩小）；抠干净透明边；一致性靠
固定 seed/参考图/同 LoRA —— 程序化生成器天然满足一致性与可复现，是 v1 选择它的主因。

### §5 落地清单（v1 已完成项）

1. ✅ 像素规格表：32×32 角色格、锚点 (16,29)。
2. ✅ `assets/textures/player/` + 命名规范 `player_<state>_<NN>.png`。
3. ✅ 全局纹理导入预设统一 Nearest / 无 mipmap / 无损（项目级 + 单文件）。
4. ✅ 动画走图集 + `Sprite2D/AtlasTexture`（单帧 PNG 保留为可替换源）。
5. ✅ 校验环节：`validate.py` 7 类断言 + `ascii_view.py` 目视。
