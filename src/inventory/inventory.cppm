// mcpp-kaki inventory module — Item / Inventory / ItemDatabase.
module;

#include <vector>

#include <godot-cpp-m/macros.h>
#include <godot_cpp/templates/hash_map.hpp> // HashMap 不被模块重导出，保持文本包含
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/color.hpp>

export module mcpp_kaki.inventory;

import godot_cpp;

namespace godot {

// 品级色（全项目统一约定）：0凡=白 1灵=蓝 2地=紫 3天=金 4仙=仙青
// ItemPickup 光柱/本体染色、背包/仓库/商店格子染色共用此一口径
export inline Color grade_color(int p_grade) {
	switch (p_grade) {
		case 1:  return Color(0.35f, 0.65f, 1.00f); // 灵·蓝
		case 2:  return Color(0.75f, 0.40f, 0.95f); // 地·紫
		case 3:  return Color(1.00f, 0.80f, 0.25f); // 天·金
		case 4:  return Color(0.55f, 1.00f, 0.80f); // 仙·仙青
		default: return Color(0.85f, 0.85f, 0.85f); // 凡·白
	}
}

// 品级格子底色（GridList 条目 bg_color 用：淡染 alpha=0.30 保证名字可读；凡品透明=保持默认底）
export inline Color grade_bg_color(int p_grade) {
	Color c = grade_color(p_grade);
	c.a = (p_grade > 0) ? 0.30f : 0.0f;
	return c;
}

// Data-only item definition. Items are registered in ItemDatabase.
// This is a plain struct, not a Godot class — no _bind_methods needed.
export struct Item {
	enum Type {
		CONSUMABLE,  // Usable from inventory (pills, elixirs)
		MATERIAL,    // Crafting/currency (spirit stones, ore)
		KEY_ITEM,    // Quest/progression items
		EQUIPMENT,   // Equippable: weapons, armor, accessories
	};

	enum EquipSlot {
		SLOT_NONE = -1,
		SLOT_WEAPON = 0,
		SLOT_ARMOR = 1,
		SLOT_ACCESSORY = 2,
	};

	StringName id;
	String name;
	String description;
	Type type = CONSUMABLE;
	int max_stack = 99;
	EquipSlot equip_slot = SLOT_NONE;

	// ---- Consumable effects (only relevant for CONSUMABLE type) ----
	float heal_amount = 0.0f;
	float heal_pct = 0.0f;      // 按比例回血（0.5 = 50%）
	float mana_amount = 0.0f;   // 回灵力（法力池）
	float energy_amount = 0.0f; // 修为经验（accumulate_energy，到顶卡境界）
	float fullness_amount = 0.0f; // 回饱食度（食物类，随境界倍率：凡人1.0/炼气1.2；辟谷后转纯 buff）
	StringName buff_id;         // 增益 buff（BuffSystem def id）
	StringName learn_skill;     // 使用后习得技能（SkillSystem def id，秘籍/残卷类）
	StringName learn_artifact;  // 使用后获得法宝（ArtifactSystem id，法宝残篇类）
	int currency_tier = -1;     // 灵石档位：-1=普通物；0下品 1中品 2上品 3极品（拾取直入钱包不进背包）
	int grade = 0;              // 品级：0凡 1灵 2地 3天 4仙
	int buy_price = 0;          // 商店买价（玩家付灵石；0=商店不售）
	int sell_price = 0;         // 商店卖价（玩家得灵石；0=不可卖）
	float breakthrough_bonus = 0.0f; // 机缘突破事件的加成（事件系统实现后生效）
	bool plantable = false;     // 可种植（草药类，洞天内灵田播种）
	int grow_seconds = 0;       // 成熟所需现实秒数（plantable 时有效）

	// ---- Equipment bonuses (only relevant for EQUIPMENT type) ----
	float attack_bonus = 0.0f;   // flat damage added
	float defense_bonus = 0.0f;  // flat damage reduction
	float speed_bonus = 0.0f;    // multiplier on move speed (0.05 = +5%)
	float elem_resist[8] = {};   // 元素抗性（按 Element 枚举下标；避水珠 ELEM_SHUI=3 水抗）
};

// Slot-based inventory container. Owned by Player.
export class Inventory : public Object {
	GDCLASS(Inventory, Object);

public:
	// 凡人背包有限（24格）；炼气事件解锁纳戒/储物袋后容量无限（UNLIMITED_SLOTS）
	static constexpr int DEFAULT_SLOTS = 24;
	static constexpr int UNLIMITED_SLOTS = 999;

	Inventory();
	~Inventory();

	// 纳戒解锁：容量 24 → 无限
	void unlock_unlimited() { _capacity = UNLIMITED_SLOTS; }
	bool is_unlimited() const { return _capacity > DEFAULT_SLOTS; }
	int get_capacity() const { return _capacity; }

	bool add_item(const StringName &p_id, int p_qty = 1);
	bool remove_item(const StringName &p_id, int p_qty = 1);
	int get_item_count(const StringName &p_id) const;
	bool has_item(const StringName &p_id) const;
	bool use_item(int p_slot_index);
	int get_free_slot_count() const;
	Dictionary get_slot(int p_idx) const;
	void clear();
	void set_slot(int p_idx, const StringName &p_id, int p_qty);

	// 装备强化：额外攻/防加成（按 item_id 跟踪，随存档持久化）
	int get_item_extra_atk(const StringName &p_id) const;
	int get_item_extra_def(const StringName &p_id) const;
	bool upgrade_item(const StringName &p_id, int p_atk_inc, int p_def_inc);
	// 强化存档序列化
	Dictionary save_extra_bonuses() const;
	void load_extra_bonuses(const Dictionary &p_data);

protected:
	static void _bind_methods();

private:
	struct Slot {
		StringName item_id;
		int quantity = 0;

		bool is_empty() const { return quantity <= 0 || item_id == StringName(); }
		void clear() {
			item_id = StringName();
			quantity = 0;
		}
	};

	Slot _slots[UNLIMITED_SLOTS];
	int _capacity = DEFAULT_SLOTS;

	// 装备强化额外加成：item_id → {extra_atk, extra_def}（随存档持久化）
	struct ExtraBonus {
		int atk = 0;
		int def = 0;
	};
	HashMap<StringName, ExtraBonus> _extra_bonuses;

	int _find_stackable_slot(const StringName &p_id) const;
	int _find_empty_slot() const;
	void _emit_changed();
};

// Singleton registry of all item definitions in the game.
export class ItemDatabase : public Node {
	GDCLASS(ItemDatabase, Node);

public:
	static ItemDatabase *get_singleton() { return _singleton; }

	ItemDatabase();
	~ItemDatabase();

	const Item *get_item(const StringName &p_id) const;
	int get_item_count() const { return _items.size(); }
	bool has_item(const StringName &p_id) const { return _items.has(p_id); }
	Dictionary get_item_info(const StringName &p_id) const;

	// 所有可播种草药 id（品级升序：凡→灵→地）——洞天灵田播种选种用
	std::vector<StringName> get_plantable_ids() const;

	void _ready() override;

protected:
	static void _bind_methods();

private:
	static ItemDatabase *_singleton;

	HashMap<StringName, Item> _items;

	void _register_items();
};

} // namespace godot
