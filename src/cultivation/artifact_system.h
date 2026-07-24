#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include "../combat/damage_types.h"
#include "../combat/skill_system.h"

namespace godot {

class Player;

// 法宝系统（design/gongfa-skills.md 第八节，已定稿）：
//   - 法宝 ≠ 装备：独立法宝栏，不占装备三槽；飞升前 本命×1+次要×2，飞升后 +3
//   - 威力系数：本命 1.2~2.0（温养/觉醒，Player 已有）；次要 1.0→1.2→1.5（两段温养）
//   - 祭出（主动）：攻击型法宝 = 一个特殊技能，复用 Skill 效果执行管线
//     （Player::exec_skill_* → DamageCalculator 结算），耗灵力+冷却
//   - B 键法宝页：技能键整页切换为法宝快捷键（页机制在 Player，槽 0..5 ↔ A/S/D/F/G/H）
class ArtifactSystem : public Object {
	GDCLASS(ArtifactSystem, Object)

public:
	enum Kind {
		KIND_ATTACK = 0, // 攻击型：可祭出
		KIND_SUPPORT,    // 辅助型：常驻被动
	};

	struct Def {
		const char *id;
		const char *name;
		Kind kind;
		// 祭出技能参数（KIND_ATTACK；复用 Skill 管线）
		DamageCategory category;
		Element element;
		float mana_cost;
		float cooldown;
		float power; // × 攻击面板 × 法宝系数
		SkillSystem::EffectKind effect;
		float proj_speed;
		Color proj_color;
		// 常驻被动（KIND_SUPPORT）：防御加成比例（× 系数后生效）
		float passive_def;
	};

	static const int MAX_SLOTS = 6; // 0=本命 1..2=次要(飞升前) 3..5=次要(飞升后)

	static const Def *find_def(const StringName &p_id);
	static String kind_name(Kind p_k);

	void set_player(Player *p) { _player = p; }

	// 获得 / 装配（槽 0 = 本命，与 Player 本命法宝同步；飞升前可换一次由 Player 约束）
	bool acquire(const StringName &p_id);
	bool is_owned(const StringName &p_id) const;
	bool equip(int p_slot, const StringName &p_id);

	// 祭出：检查 已装备/栏位上限/冷却/灵力 → 效果×法宝系数 → 喂养功法+温养
	bool activate_slot(int p_slot);

	// 温养（击杀/祭出推进）：本命走 Player，次要存自身
	void nurture_equipped(float p_amount);
	float get_slot_coeff(int p_slot) const; // 本命 1.2~2.0 / 次要 1.0→1.5
	// 辅助型常驻被动汇总（装备中的 SUPPORT 法宝：Σ passive_def × coeff）
	float get_passive_def_bonus() const;

	int get_slot_limit() const; // 飞升前 3 / 飞升后 6（问 Player）
	StringName get_slot_artifact(int p_slot) const;
	// 绑定给 HUD/菜单: {id,name,kind,kind_name,coeff,cd_remaining,cooldown,mana_cost,power,locked}
	Dictionary get_slot_info(int p_slot) const;
	Array get_owned_list() const;

	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

	// 信号: artifact_activated(id) / artifacts_changed
protected:
	static void _bind_methods();

private:
	Player *_player = nullptr;
	HashSet<StringName> _owned;
	StringName _slots[MAX_SLOTS]; // _slots[0] 镜像 Player 本命（不持久化于此）
	HashMap<StringName, float> _nurture;        // 次要法宝温养（本命在 Player）
	HashMap<StringName, double> _cooldown_until;

	double _now() const;
	static constexpr float NURTURE_STAGE1 = 300.0f; // → 1.2
	static constexpr float NURTURE_STAGE2 = 600.0f; // → 1.5
};

} // namespace godot

VARIANT_ENUM_CAST(godot::ArtifactSystem::Kind);
