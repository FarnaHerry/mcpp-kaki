#ifndef CPP_KAKI_SOUL_LEDGER_SYSTEM_H
#define CPP_KAKI_SOUL_LEDGER_SYSTEM_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../utils/text.h"

namespace godot {

class Player;
class Enemy;

// 生死簿：独立数据系统（不塞进 CultivationSystem，design/cultivation-realms.md §四/§五）。
// 每生灵一条记录：出身/原身/寿元/定数。簿上寿元 = 物种默认（凡人 100 年），
// 实际寿元随境界拉大——「修仙增寿不同步地府」的信息差是勾魂错抓的默认状态。
// 职责：①生死簿数据（查簿/改簿/免死）②濒死时刷出黑白无常（勾魂）③反杀奖励。
class SoulLedgerSystem : public Node {
	GDCLASS(SoulLedgerSystem, Node);

public:
	// 实际寿元表（按 Realm index）：凡人100/炼气150/筑基250/金丹500/元婴2000/化神5000…
	// 超出前几境走数组兜底（越后期寿元越长）。
	static int lifespan_for_realm(int p_realm);

	void set_player(Player *p);
	Player *get_player() const { return _player; }

	int get_ledger_lifespan() const { return _ledger_lifespan; }
	void set_ledger_lifespan(int v);
	int get_actual_lifespan() const { return lifespan_for_realm(_realm_cache); }

	String get_original_body() const { return _original_body; }
	void set_original_body(const String &v);
	String get_origin_name() const; // 出身名（后天修炼/先天神圣，读 CultivationSystem）

	bool has_soul_protection() const { return _soul_protection; }
	bool mark_soul_exempt();        // 改簿划名 → 免死一次 + 阴寿豁免（永久）
	bool consume_soul_protection(); // 死亡时消耗：true=本次免死（清免死标记）
	bool is_struck() const { return _struck; } // 划名永久标记：脱离生死轮回，不再被勾魂

	bool is_reaper_active() const { return _reaper_active; }
	bool was_killed_by_reaper(Node *p_source); // 勾魂使击杀判定

	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

	void _ready() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	Player *_player = nullptr;
	int _ledger_lifespan = 100;    // 簿上寿元（物种默认）
	int _realm_cache = 0;          // 实际寿元按境界缓存
	String _original_body = TXT("凡人");
	bool _soul_protection = false; // 免死一次标记（划名后置，死亡消耗）
	bool _struck = false;          // 划名永久标记：脱离生死轮回，不再被勾魂（阴寿豁免）

	Enemy *_reaper_a = nullptr;
	Enemy *_reaper_b = nullptr;
	bool _reaper_active = false;   // 黑白无常存活中
	bool _reaper_spawned = false;  // 本轮濒死已触发（回血>50% 复位）
	float _tip_t = 0.0f;           // 提示自动消隐

	void _emit_lifespan();
	void _spawn_reapers();
	Enemy *_spawn_one_reaper(Node *root, const String &name, const Vector2 &pos, const Color &tint);
	void _on_enemy_killed(Node *enemy, Node *killer);
	void _on_realm_changed(int p_old, int p_new, const String &p_name);
	void _on_player_respawned();
	void _show_tip(const String &text, float seconds);
};

} // namespace godot

#endif // CPP_KAKI_SOUL_LEDGER_SYSTEM_H
