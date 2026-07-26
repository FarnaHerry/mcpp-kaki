#pragma once

#include <vector>

#include <vector>

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

class Player;

// 炼丹系统（design/alchemy.md 第三节，已定稿）：
//   - 丹炉随身（纳戒设定），ESC 菜单炼丹页操作，随时随地可炼
//   - 固定配方 v1；成功率字段预留失败机制（v1 数值全 100%）
//   - 地品配方境界门控（金丹起），灰显标注
//   - 炼制 = 练气行为：每炉喂练气熟练 +5
class AlchemySystem : public Object {
	GDCLASS(AlchemySystem, Object)

public:
	static constexpr int MAX_MATS = 3;

	struct Recipe {
		const char *id;             // 产物物品 id
		const char *name;           // 丹药名（展示用，与 ItemDatabase 一致）
		const char *mat_id[MAX_MATS];
		int mat_qty[MAX_MATS];
		int mat_count;
		int grade;                  // 品级 0凡/1灵/2地
		int min_realm;              // 境界门控（金丹=3）
		float success_rate;         // v1 全 1.0（失败机制预留）
		const char *effect_desc;
	};

	static const Recipe *find_recipe(const StringName &p_id);
	static void ensure_loaded();
	static int get_recipe_count();
	static const Recipe *get_recipe(int p_idx);

	void set_player(Player *p) { _player = p; }

	// 材料是否足够（境界门控独立判断，见 is_realm_locked）
	bool can_craft(const StringName &p_id) const;
	bool is_realm_locked(const Recipe *p_r) const;
	// 炼制：校验 → 扣材料 → 成功率 roll → 产物入包；每炉喂练气 +5
	bool craft(const StringName &p_id);
	// 配方列表（菜单页用）：[{id,name,grade,effect,mats:[{id,name,need,have}],can_craft,realm_locked}]
	Array get_recipe_list() const;

	String get_last_message() const { return _last_message; }

protected:
	static void _bind_methods();

private:
	static std::vector<Recipe> s_recipes;
	static bool s_loaded;
	Player *_player = nullptr;
	String _last_message;
};

} // namespace godot
