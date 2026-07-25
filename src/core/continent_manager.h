#ifndef CPP_KAKI_CONTINENT_MANAGER_H
#define CPP_KAKI_CONTINENT_MANAGER_H
#include <vector>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {

class Player;

// 洲框架（design/world-map.md 后西游·四大部洲）：洲注册表 + 当前洲追踪 + 旅行入口。
// 洲 = 独立根场景；旅行走 GameManager 静态桥（collect_save_data → 切场景 → 新场景应用）。
// 每洲场景的 bootstrap 都创建一个 ContinentManager；当前洲由当前场景路径反推。
class ContinentManager : public Node {
	GDCLASS(ContinentManager, Node);

public:
	struct Def {
		const char *id;
		const char *name;
		const char *scene;
		float spawn_x;
		float spawn_y;
		int min_realm;      // 解锁境界（Realm 枚举值：金丹3 炼虚6 渡劫9）
		const char *desc;
		const char *gate;   // 门控显示文本（空 = 无门控）
	};

	static const Def *find_def(const String &p_id);
	static void ensure_loaded();

	String get_current_id() const { return _current_id; }
	String get_current_name() const;
	bool is_unlocked(const String &p_id) const; // 境界够即解锁（读玩家境界）
	bool can_travel(const String &p_id) const;  // 已解锁且非当前洲
	bool travel_to(const String &p_id);         // 旅行（经云海强渡 → 登岸 complete_travel）
	bool travel_to_direct(const String &p_id);  // 直达（调试/harness，跳过云海）
	bool complete_travel();                     // 云海登岸：前往 cp.travel_dest 记载的目的洲
	Array get_continent_list() const;           // 云游图数据源

	void _ready() override;

protected:
	static void _bind_methods();

private:
	static std::vector<Def> s_defs;
	static bool s_loaded;
	String _current_id;

	Player *_find_player() const;
};

} // namespace godot

#endif // CPP_KAKI_CONTINENT_MANAGER_H
