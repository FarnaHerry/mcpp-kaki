#ifndef CPP_KAKI_ENEMY_DATABASE_H
#define CPP_KAKI_ENEMY_DATABASE_H

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

namespace godot {

// EnemyDatabase — 敌人定义表（design/data-externalization.md 敌人数据化）。
// 纯 static 工具类（无需 GDREGISTER）：data/enemies.json 优先 + 内置硬编码兜底。
// 消费方：Enemy::set_enemy_id（定义自动应用）/ world_common.spawn_enemy_by_id。

struct EnemyDef {
	String id;
	String name;             // 中文显示名（display_name / Boss 血条标题）
	float hp = 1.0f;         // 基础血（Boss 未 ×5，set_is_boss 的幂等补偿负责 ×5）
	float atk = 10.0f;
	float speed = 60.0f;
	float detection = 200.0f;
	float attack_range = 35.0f;
	float attack_cooldown = 0.8f;
	float preferred = 0.0f;  // 远程理想距离（0=近战）
	int realm = 0;
	bool ranged = false;
	bool flying = false;
	bool boss = false;
	Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
	Vector2 size = Vector2(20, 28);
	String drops;            // 命名掉落表（空串=走类别兜底掉落表）
	float elite_chance = 0.0f; // 生成时自动精英化概率（0=不精英化；Boss 不配）
};

class EnemyDatabase {
public:
	static void ensure_loaded();                       // 惰性加载（JSON + 兜底合并）
	static const EnemyDef *get_def(const String &p_id); // 未找到返回 nullptr

private:
	static std::vector<EnemyDef> s_defs;
	static bool s_loaded;

	static void _load_hardcoded();
	static void _apply_json(); // JSON 覆盖/新增（同 id 按字段覆盖）
};

// AffixDatabase — 精英词缀表（data/affixes.json 优先 + 硬编码兜底）。
// 消费方：Enemy::make_elite / make_elite_random。
struct AffixDef {
	String id;
	String name;                               // 中文词缀名（display_name 前缀用）
	float hp_mult = 1.0f;
	float atk_mult = 1.0f;
	float speed_mult = 1.0f;
	float detect_mult = 1.0f;                  // 侦测半径倍率（迅捷用）
	float def_add = 0.0f;                      // 物理防御平加（厚甲用）
	Color tint = Color(1.0f, 1.0f, 1.0f, 1.0f); // 本体染色
};

class AffixDatabase {
public:
	static void ensure_loaded();
	static const AffixDef *get_affix(const String &p_id); // 未找到返回 nullptr
	static const AffixDef *random_affix();                // 均匀随机（表空返回 nullptr）

private:
	static std::vector<AffixDef> s_affixes;
	static bool s_loaded;

	static void _load_hardcoded();
	static void _apply_json();
};

} // namespace godot

#endif // CPP_KAKI_ENEMY_DATABASE_H
