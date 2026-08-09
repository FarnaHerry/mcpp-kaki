# Session 015 — QWERTY+ASDFGH 全技能键位重构（12 技能槽）

**日期**: 2026-08-10
**分支**: main
**起始提交**: c55effa（session-014 文档）
**结束提交**: （本会话提交）

## 缘起

用户：「现在没有用上 QWER 那一行按钮，一并用上，能塞更多技能」。先摸清现状后用户定夺布局：
「**QWERTY+ASDFGH 都作技能键，紧凑些；其他（功能键）移到 UIOPJKL，ZXCVBNM 保留基础功能/切换**」。
追问澄清：用户明确**重构只针对正常游戏**——菜单/仓库/配置等页面里的 Q/E 翻页等**不需要改、不冲突**
（菜单打开即暂停，Q/E 在「菜单翻页」与「正常游戏技能键」两态复用同一物理键）。

## 最终键位

```
技能区  Q W E R T Y ┐ 12 技能槽（slot 0..5 上行 / 6..11 下行）
        A S D F G H ┘
功能区  U=威压  I=背包  O=洞天  P=灵压  L=打坐
基础区  Z=冲刺 X=攻击+交互 C=跳跃 B=切法宝页  (V/K/N/M 空预留)
        ESC=菜单  1~6=消耗品  Space=确认副键  ↑↓←→=移动/门口进出
```

- **槽位 8→12**：QWERTY 上行 0..5 + ASDFGH 下行 6..11；G/H 原闲置槽激活。
- **类型分组**（`slot_type()` 放松门控）：Q/W/A/S 武技、E/R/D/F 法术、T/Y/G 神通、H 仙法。
- **挪键**：打坐 Q→**L**、威压 V→**U**、灵压 R→**P**；背包 I、洞天 O、B 切页**不动**。
- **翻页保留 Q/E**：菜单/仓库等页面翻页不改（用户澄清不冲突）。

## 改动

### project.godot（input map）
- 新建 4 action：`skill_q/w/e/r`（物理键 81/87/69/82）。
- 重绑：`cultivate` Q→L、`pressure_wei` V→U、`pressure_lin` R→P。`inventory`/`dongtian` 不动。
- python 解析校验：字母区每键恰好 1 个 action，唯一 X=攻击+交互合一是预期设计。

### 槽位扩展（combat.cppm / skill_system.cpp / player.cpp）
- `SLOT_COUNT` 8→12，`slot_type()` 重写为上述 12 槽分组映射。
- `player.cpp` 技能输入块 `SLOT_ACTIONS[12]`=q/w/e/r/t/y/a/s/d/f/g/h；法宝页判定 `i>=6`（下行 A~H=法宝槽0..5）。
- **初始装配索引迁移**：破空/突进 0/1→6/7(A/S)、火弹/冰锥 2/3→8/9(D/F)、缩地 6→4(T)、天雷引 7→**11**(H，新仙法槽)。
- **存档迁移**：`load_from_dict` 检测旧 8 槽档（`slots.size()<SLOT_COUNT`）→ `OLD8_TO_NEW[8]={6,7,8,9,-1,-1,4,5}` 重排
  （A/S/D/F→6..9、G/H 弃、T/Y→4/5），新档直读。防旧档技能错位。

### HUD 技能栏（game_hud.cpp）
- `_create_skill_bar` 重写为**两排紧凑居中**：QWERTY 上排(y=222)/ASDFGH 下排(y=246)，各 6 槽×20px，
  行宽 130px 居中 x0=175（不与左下消耗品栏 x=8..146 重叠）。
- `_update_skill_bar` 数据源：法宝页 `i>=6`（下行 ASDFGH）→ `arts->get_slot_info(i-6)`；其余技能槽。

### 技能页装配（game_menu.cpp）
- `_handle_skill_input` 装配键 6→**12**（SLOT_ACTIONS/IDX/KEYS 全 QWERTY+ASDFGH）。
- 槽位总览重写：按类型 4 组（武技 Q/W/A/S、法术 E/R/D/F、神通 T/Y/G、仙法 H）分四列展示。
- 提示文案改「QWERTY/ASDFGH 装入对应槽（类型须匹配）」。

### 文案/翻译同步
- HUD：「机缘已至 [Q]」→「[L]」、威压「V」→「U」、灵压「R」→「P」。
- game_menu 能力页战技分区：威压 U / 灵压 P。
- `data/locale_en.json` 6 条翻译 key 同步（中文原文作 key，原文改则 key 改）。
- CLAUDE.md Input Map 段整体重写 + SkillSystem/HUD 技能栏/威压灵压条目更新。

## 测试

- 新增 `scripts/test_skill_qwerty.gd`（**22 PASS**）：12 槽 assign 全类型、类型门控拒装
  （武技装法术槽/仙法槽拒）、cast_slot、skill_q action 输入管线、存档 12 槽数组往返。
- 迁移既有测试索引：test_skills.gd（7 处）/ test_skills2.gd（12 处 + 天雷引 H 槽）/
  test_skill_page.gd（A 槽 0→6、D 槽 2→8）。
- 全量回归见末行。

## 关键坑

- **仙法槽位移**：旧仙法在 slot7(Y)，新映射 slot7=S=武技——天雷引若不迁会 `assign` 失败（类型不符）。
  迁到 slot11(H) 新仙法槽。
- **测试断言的是自动装配槽位**：开局/境界授予走 player.cpp 硬编码 `assign(索引)`，改映射必须同步这些字面量，
  否则测试读不到（test_skills 的 slot0=破空斩 这类断言全踩）。
- **菜单 Q/E 翻页不动**：用户明确正常游戏技能键与菜单翻页分态复用，无需让位——避免过度重构。

## 合并后回归

全量复跑 **51 PASS / 0 FAIL**（修正判定：旧 grep 把 `DONE fail=N` 误判为过，补 fail=[1-9] 检测后
test_shentong/test_shenwai_clone 的旧索引失败才暴露）。另发现两处**守卫条件漏迁**：
player.cpp 授予点 `get_slot_skill(6/2/3/7)` 空槽判断仍查旧索引（6 已变 A 武技槽装着破空斩 → 非空 → 跳过 assign），
导致化神缩地成寸/炼气火弹冰锥/真仙天雷引授予后不落槽——一并迁到目标槽（4/8/9/11）。
