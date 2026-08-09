#ifndef CPP_KAKI_DONGTIAN_MANAGER_H
#define CPP_KAKI_DONGTIAN_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../utils/text.h"

namespace godot {

class Player;
class CameraRoom2D;

// 洞天系统 v1（后花园·空洞天）— design/dongtian.md
// 玩家随身小世界：炼虚解锁 dongtian 能力后，安全状态按 O 键进出。
// 进出模式复用 Portal 经验（挂子场景 + 玩家重挂载 + 相机锁定），
// 但无固定点——任意安全点进出，退出回到进入时的位置和场景。
class DongtianManager : public Node {
	GDCLASS(DongtianManager, Node);

public:
	DongtianManager();

	void set_player(Player *p) { _player = p; }
	void set_camera(CameraRoom2D *c) { _camera = c; }

	bool is_inside() const { return _inside; }
	Vector2 get_return_position() const { return _saved_world_pos; }

	// ---- 灵田（v2 种植，design/dongtian.md）----
	// 状态自持于 Manager（洞天场景卸载后生长不丢），现实时间生长。
	// v4 扩张：6 → 最多 12 块，扩张碑灵石购买（价格递增，下品基准）。
	static constexpr int BASE_PLOTS = 6;
	static constexpr int MAX_PLOTS = 12;

	// 地块状态：{empty, herb, herb_name, mature, remaining}（empty=true 时其余无意义）
	Dictionary get_plot(int p_index) const;
	// 背包里第一种可播种草药（空 = 无可播种）
	StringName get_first_plantable() const;
	// 播种/收获（收获返回数量，未成熟/空地返回 0）
	bool plant(int p_index);
	int harvest(int p_index);
	// 测试用：拨快生长（planted_at 回拨 seconds 秒）
	void debug_age_plot(int p_index, double p_seconds);

	// ---- v4 灵田扩张 ----
	int get_plot_count() const { return _plot_count; }
	// 下一块地的价格（下品基准）；已满（12 块）返回 0
	int get_expand_cost() const;
	// 购买下一块地（走 CurrencySystem 扣款，不足/已满返回 false）
	bool expand_plot();

	// ---- v4 聚灵阵升级（两级，每级打坐倍率 +0.5）----
	static constexpr int JLZ_MAX_LEVEL = 2;
	int get_jlz_level() const { return _jlz_level; }
	// 下一级价格（下品基准：500 / 1500）；满级返回 0
	int get_jlz_upgrade_cost() const;
	// 升级聚灵阵（走 CurrencySystem 扣款，不足/满级返回 false）
	bool upgrade_jlz();
	// 打坐倍率加成（Player::get_dongtian_meditate_mult 叠加）
	double get_jlz_bonus() const { return 0.5 * _jlz_level; }

	// ---- 设施补全：灵植采集点×2（现实时间刷新，状态自持于 Manager）----
	// 与灵田不同：采集点草药固定（聚灵草/千年灵芝），采后枯萎，刷新时长到点复生。
	static constexpr int HERB_SPOTS = 2;

	// 采集点状态：{herb, herb_name, qty, refresh, available, remaining}
	Dictionary get_herb_spot(int p_index) const;
	// 采集（枯萎/未刷新返回 false）：入包 + 喂练气
	bool gather_herb_spot(int p_index);
	// 测试用：拨快刷新（harvested_at 回拨 seconds 秒）
	void debug_age_herb_spot(int p_index, double p_seconds);

	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

	// ---- 仓库（v2 储物箱，与纳戒互补）----
	// 槽位同样自持于 Manager 并随档持久化。
	static constexpr int STORAGE_SLOTS = 48;

	// 储物槽状态：{} = 空格，否则 {id, quantity, name}
	Dictionary get_storage_slot(int p_index) const;
	// 背包整堆存入（叠放同类或首空格；放不下的部分留在背包）。返回实际存入数量
	int deposit_from_player(int p_inv_slot);
	// 储物槽整堆取出给玩家。返回实际取出数量
	int withdraw_to_player(int p_storage_slot);

	// 读档专用：只把玩家挪回主场景根并卸载洞天，不恢复位置（由读档回填）
	void force_exit_for_load();

	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	static constexpr const char *DONGTIAN_SCENE = "res://scenes/rooms/dongtian.tscn";
	// 云海强渡途中禁入（坠海遣返等机制与洞天往返冲突）
	static constexpr const char *YUNHAI_SCENE = "res://scenes/continents/yunhai.tscn";

	Rect2 _bounds = Rect2(0, 0, 480, 270);

	Player *_player = nullptr;
	CameraRoom2D *_camera = nullptr;
	Node *_loaded_scene = nullptr;
	bool _inside = false;
	Vector2 _saved_world_pos;

	float _hint_t = 0.0f; // 原因提示自动消隐计时

	// 灵田地块：herb 为空 = 空地；planted_at = Unix 时间戳（现实时间生长）
	struct Plot {
		StringName herb;
		int64_t planted_at = 0;
	};
	Plot _plots[MAX_PLOTS];
	int _plot_count = BASE_PLOTS; // v4：已开辟地块数（6→12）
	int _jlz_level = 0;           // v4：聚灵阵等级（0→2，每级打坐 +0.5）
	static int64_t _now();

	// 仓库槽位：item 空 = 空格
	struct StorageSlot {
		StringName item;
		int qty = 0;
	};
	StorageSlot _storage[STORAGE_SLOTS];

	// 灵植采集点：herb 固定（见 cpp 定义表）；harvested_at=0 = 可采集
	struct HerbSpot {
		StringName herb;
		int64_t harvested_at = 0;
	};
	HerbSpot _herb_spots[HERB_SPOTS];

	void _try_enter();
	void _enter();
	void _exit(bool p_restore_pos);
	void _show_reason(const String &p_text);
	bool _in_combat() const;
};

} // namespace godot

#endif // CPP_KAKI_DONGTIAN_MANAGER_H
